#include "LowUtilFileSystem.h"

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <mutex>
#include <thread>

namespace Low {
  namespace Util {
    namespace FileSystem {
      static std::wstring toWide(const std::filesystem::path &p)
      {
        // On Windows, filesystem::path::wstring() is what you want.
        return p.wstring();
      }

      static std::filesystem::path toFsPath(const std::wstring &w)
      {
        return std::filesystem::path(w);
      }

      // ---- Optional ReadDirectoryChangesExW support (dynamic
      // lookup) ----

      // Some SDKs may not have these defined, but the function exists
      // on Win10 1709+. We’ll define the enum + structs we need.

      typedef enum _READ_DIRECTORY_NOTIFY_INFORMATION_CLASS
      {
        ReadDirectoryNotifyInformation = 1,
        ReadDirectoryNotifyExtendedInformation = 2
      } READ_DIRECTORY_NOTIFY_INFORMATION_CLASS;

      using ReadDirectoryChangesExW_t = BOOL(WINAPI *)(
          HANDLE, LPVOID, DWORD, BOOL, DWORD, LPDWORD, LPOVERLAPPED,
          LPOVERLAPPED_COMPLETION_ROUTINE,
          READ_DIRECTORY_NOTIFY_INFORMATION_CLASS);

      static ReadDirectoryChangesExW_t loadReadDirectoryChangesExW()
      {
        HMODULE h = ::GetModuleHandleW(L"Kernel32.dll");
        if (!h)
          return nullptr;
        return reinterpret_cast<ReadDirectoryChangesExW_t>(
            ::GetProcAddress(h, "ReadDirectoryChangesExW"));
      }

      static Watcher::EventType mapAction(DWORD action)
      {
        switch (action) {
        case FILE_ACTION_ADDED:
          return Watcher::EventType::Added;
        case FILE_ACTION_REMOVED:
          return Watcher::EventType::Removed;
        case FILE_ACTION_MODIFIED:
          return Watcher::EventType::Modified;
        // rename old/new handled by pairing logic
        case FILE_ACTION_RENAMED_OLD_NAME:
        case FILE_ACTION_RENAMED_NEW_NAME:
        default:
          return Watcher::EventType::Unknown;
        }
      }

      struct Watcher::Impl
      {
        std::filesystem::path root;
        std::wstring rootW;

        HANDLE dir = INVALID_HANDLE_VALUE;
        HANDLE iocp = nullptr;

        OVERLAPPED ov{};
        List<std::byte> buffer;

        std::atomic<bool> running{false};
        std::thread thread;

        std::mutex mtx;
        List<Watcher::Event> events;

        // rename pairing (best-effort)
        std::optional<std::filesystem::path> pendingRenameOld;

        ReadDirectoryChangesExW_t pReadDirChangesExW = nullptr;
        bool usingExtendedInfo = false;

        bool start(const std::filesystem::path &rootPath)
        {
          stop();

          root = std::filesystem::absolute(rootPath);
          rootW = toWide(root);

          // 64KB is usually plenty; you can tune this.
          buffer.resize(64 * 1024);

          dir = ::CreateFileW(
              rootW.c_str(), FILE_LIST_DIRECTORY,
              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
              nullptr, OPEN_EXISTING,
              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
              nullptr);
          if (dir == INVALID_HANDLE_VALUE) {
            return false;
          }

          iocp = ::CreateIoCompletionPort(dir, nullptr, 1, 0);
          if (!iocp) {
            ::CloseHandle(dir);
            dir = INVALID_HANDLE_VALUE;
            return false;
          }

          pReadDirChangesExW = loadReadDirectoryChangesExW();
          usingExtendedInfo = (pReadDirChangesExW != nullptr);

          running = true;

          if (!armRead()) {
            stop();
            return false;
          }

          thread = std::thread([this] { loop(); });
          return true;
        }

        void stop()
        {
          if (!running.exchange(false)) {
            // Already stopped
          }

          if (dir != INVALID_HANDLE_VALUE) {
            ::CancelIoEx(dir, &ov);
          }

          if (iocp) {
            ::PostQueuedCompletionStatus(iocp, 0, 0, nullptr);
          }

          if (thread.joinable()) {
            thread.join();
          }

          if (iocp) {
            ::CloseHandle(iocp);
            iocp = nullptr;
          }

          if (dir != INVALID_HANDLE_VALUE) {
            ::CloseHandle(dir);
            dir = INVALID_HANDLE_VALUE;
          }

          std::memset(&ov, 0, sizeof(ov));
          pendingRenameOld.reset();
          usingExtendedInfo = false;
          pReadDirChangesExW = nullptr;
          buffer.clear();
          root.clear();
          rootW.clear();

          // Keep events? Usually no; but safe either way.
          // We'll keep them and let caller poll after stop if
          // desired.
        }

        bool armRead()
        {
          std::memset(&ov, 0, sizeof(ov));

          constexpr DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME |
                                   FILE_NOTIFY_CHANGE_DIR_NAME |
                                   FILE_NOTIFY_CHANGE_LAST_WRITE |
                                   FILE_NOTIFY_CHANGE_SIZE |
                                   FILE_NOTIFY_CHANGE_CREATION;

          DWORD bytesReturned = 0;

          if (usingExtendedInfo) {
            // ReadDirectoryChangesExW produces
            // FILE_NOTIFY_EXTENDED_INFORMATION records.
            BOOL ok = pReadDirChangesExW(
                dir, buffer.data(), static_cast<DWORD>(buffer.size()),
                TRUE, // watch subtree
                filter, &bytesReturned, &ov, nullptr,
                ReadDirectoryNotifyInformation);
            if (ok)
              return true;

            // If ExW fails for some reason, fall back to classic API.
            usingExtendedInfo = false;
          }

          // Classic ReadDirectoryChangesW produces
          // FILE_NOTIFY_INFORMATION records.
          BOOL ok = ::ReadDirectoryChangesW(
              dir, buffer.data(), static_cast<DWORD>(buffer.size()),
              TRUE, filter,
              &bytesReturned, // ignored for overlapped; completion
                              // gives bytes
              &ov, nullptr);

          return ok == TRUE;
        }

        void pushEvent(Watcher::Event e)
        {
          std::lock_guard<std::mutex> lock(mtx);
          events.emplace_back(std::move(e));
        }

        void noteOverflow()
        {
          Watcher::Event e;
          e.type = Watcher::EventType::Overflow;
          pushEvent(std::move(e));
        }

        void handleRenameOld(const std::filesystem::path &oldRel)
        {
          pendingRenameOld = oldRel;
        }

        void handleRenameNew(const std::filesystem::path &newRel)
        {
          if (pendingRenameOld.has_value()) {
            Watcher::Event e;
            e.type = Watcher::EventType::Renamed;
            e.oldPath = pendingRenameOld;
            e.path = newRel;
            pendingRenameOld.reset();
            pushEvent(std::move(e));
          } else {
            // No old name seen; treat as Added
            Watcher::Event e;
            e.type = Watcher::EventType::Added;
            e.path = newRel;
            pushEvent(std::move(e));
          }
        }

        void loop()
        {
          while (running.load(std::memory_order_relaxed)) {
            DWORD bytes = 0;
            ULONG_PTR key = 0;
            LPOVERLAPPED pov = nullptr;

            BOOL ok = ::GetQueuedCompletionStatus(iocp, &bytes, &key,
                                                  &pov, INFINITE);

            if (!running.load(std::memory_order_relaxed))
              break;

            if (!ok) {
              // If canceled or transient error, try to re-arm unless
              // stopping
              if (running.load(std::memory_order_relaxed)) {
                // Consider logging ::GetLastError() in debug
                armRead();
              }
              continue;
            }

            if (bytes == 0) {
              // Overflow / missed events possible -> ask higher
              // layers to rescan
              noteOverflow();
              armRead();
              continue;
            }

            parseClassic(bytes);

            armRead();
          }
        }

        void parseClassic(DWORD bytes)
        {
          size_t offset = 0;
          const std::byte *base =
              reinterpret_cast<const std::byte *>(buffer.data());

          while (offset < bytes) {
            auto *info =
                reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(
                    base + offset);

            // Basic structural validation
            const size_t header =
                offsetof(FILE_NOTIFY_INFORMATION, FileName);
            const size_t remaining = bytes - offset;

            if (remaining < header)
              break;
            if (info->FileNameLength > remaining - header) {
              // Mis-parse / corrupted buffer / overflow -> tell
              // higher layer to rescan
              noteOverflow();
              break;
            }
            if ((info->FileNameLength % sizeof(WCHAR)) != 0)
              break;

            const size_t nameWchars =
                info->FileNameLength / sizeof(WCHAR);
            std::wstring name(info->FileName,
                              info->FileName + nameWchars);

            const DWORD action = info->Action;
            auto rel = toFsPath(name);

            if (action == FILE_ACTION_RENAMED_OLD_NAME) {
              handleRenameOld(rel);
            } else if (action == FILE_ACTION_RENAMED_NEW_NAME) {
              handleRenameNew(rel);
            } else {
              Watcher::Event e;
              e.type = mapAction(action);
              e.path = rel;
              pushEvent(std::move(e));
            }

            if (info->NextEntryOffset == 0)
              break;
            offset += info->NextEntryOffset;
          }
        }

        List<Watcher::Event> poll()
        {
          std::lock_guard<std::mutex> lock(mtx);
          auto out = std::move(events);
          events.clear();
          return out;
        }
      };

      // ---- Watcher public wrapper ----

      Watcher::Watcher() : impl_(std::make_unique<Impl>())
      {
      }
      Watcher::~Watcher()
      {
        stop();
      }

      bool Watcher::start(const std::filesystem::path &root)
      {
        return impl_->start(root);
      }

      void Watcher::stop()
      {
        if (impl_)
          impl_->stop();
      }

      List<Watcher::Event> Watcher::poll()
      {
        if (!impl_)
          return {};
        return impl_->poll();
      }

      bool Watcher::running() const noexcept
      {
        return impl_ && impl_->running.load();
      }

    } // namespace FileSystem
  } // namespace Util
} // namespace Low

#endif // _WIN32
