#include "LowUtilFileSystem.h"

#if defined(__linux__)

#include <sys/inotify.h>
#include <unistd.h>

#include <cerrno>
#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace Low {
  namespace Util {
    namespace FileSystem {
      namespace {
        constexpr uint32_t WATCH_MASK =
            IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM |
            IN_MOVED_TO | IN_CLOSE_WRITE | IN_ATTRIB | IN_DELETE_SELF |
            IN_MOVE_SELF;

        static Watcher::EventType map_mask(uint32_t p_Mask)
        {
          if (p_Mask & IN_CREATE) {
            return Watcher::EventType::Added;
          }
          if (p_Mask & IN_DELETE) {
            return Watcher::EventType::Removed;
          }
          if (p_Mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB)) {
            return Watcher::EventType::Modified;
          }
          return Watcher::EventType::Unknown;
        }
      } // namespace

      struct Watcher::Impl
      {
        std::filesystem::path root;
        int fd = -1;
        bool isRunning = false;

        std::mutex mtx;
        std::unordered_map<int, std::filesystem::path> watchPaths;
        std::unordered_map<uint32_t, std::filesystem::path> pendingMoves;

        bool start(const std::filesystem::path &p_Root)
        {
          stop();

          std::error_code l_Error;
          root = std::filesystem::absolute(p_Root, l_Error);
          if (l_Error || !std::filesystem::is_directory(root)) {
            return false;
          }

          fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
          if (fd < 0) {
            return false;
          }

          isRunning = true;
          if (!add_directory_recursive(root)) {
            stop();
            return false;
          }

          return true;
        }

        void stop()
        {
          if (fd >= 0) {
            close(fd);
            fd = -1;
          }

          std::lock_guard<std::mutex> lock(mtx);
          isRunning = false;
          watchPaths.clear();
          pendingMoves.clear();
          root.clear();
        }

        bool add_directory(const std::filesystem::path &p_Path)
        {
          if (fd < 0) {
            return false;
          }

          const int l_Watch = inotify_add_watch(
              fd, p_Path.c_str(), WATCH_MASK | IN_ONLYDIR);
          if (l_Watch < 0) {
            return false;
          }

          std::lock_guard<std::mutex> lock(mtx);
          watchPaths[l_Watch] = p_Path;
          return true;
        }

        bool add_directory_recursive(const std::filesystem::path &p_Path)
        {
          add_directory(p_Path);

          std::error_code l_Error;
          std::filesystem::recursive_directory_iterator l_It(
              p_Path, std::filesystem::directory_options::skip_permission_denied,
              l_Error);
          std::filesystem::recursive_directory_iterator l_End;

          for (; !l_Error && l_It != l_End; l_It.increment(l_Error)) {
            if (l_It->is_directory(l_Error)) {
              add_directory(l_It->path());
            }
          }

          return true;
        }

        std::filesystem::path make_relative(
            const std::filesystem::path &p_Path) const
        {
          std::error_code l_Error;
          std::filesystem::path l_Rel =
              std::filesystem::relative(p_Path, root, l_Error);
          if (l_Error) {
            return p_Path.filename();
          }
          return l_Rel;
        }

        List<Watcher::Event> poll()
        {
          List<Watcher::Event> l_Events;
          if (fd < 0 || !isRunning) {
            return l_Events;
          }

          alignas(inotify_event) char l_Buffer[64 * 1024];

          for (;;) {
            const ssize_t l_Bytes =
                read(fd, l_Buffer, sizeof(l_Buffer));
            if (l_Bytes < 0) {
              if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
              }

              Watcher::Event l_Event;
              l_Event.type = Watcher::EventType::Overflow;
              l_Events.push_back(std::move(l_Event));
              break;
            }

            if (l_Bytes == 0) {
              break;
            }

            size_t l_Offset = 0;
            while (l_Offset < static_cast<size_t>(l_Bytes)) {
              const auto *l_InotifyEvent =
                  reinterpret_cast<const inotify_event *>(l_Buffer +
                                                          l_Offset);
              handle_event(*l_InotifyEvent, l_Events);
              l_Offset += sizeof(inotify_event) + l_InotifyEvent->len;
            }
          }

          return l_Events;
        }

        void handle_event(const inotify_event &p_Event,
                          List<Watcher::Event> &p_Events)
        {
          if (p_Event.mask & IN_Q_OVERFLOW) {
            Watcher::Event l_Event;
            l_Event.type = Watcher::EventType::Overflow;
            p_Events.push_back(std::move(l_Event));
            return;
          }

          std::filesystem::path l_Directory;
          {
            std::lock_guard<std::mutex> lock(mtx);
            auto l_It = watchPaths.find(p_Event.wd);
            if (l_It == watchPaths.end()) {
              return;
            }
            l_Directory = l_It->second;
          }

          const std::filesystem::path l_Name =
              p_Event.len > 0 ? std::filesystem::path(p_Event.name)
                              : std::filesystem::path();
          const std::filesystem::path l_FullPath =
              l_Name.empty() ? l_Directory : l_Directory / l_Name;

          if ((p_Event.mask & IN_ISDIR) &&
              (p_Event.mask & (IN_CREATE | IN_MOVED_TO))) {
            add_directory_recursive(l_FullPath);
          }

          if (p_Event.mask & IN_MOVED_FROM) {
            std::lock_guard<std::mutex> lock(mtx);
            pendingMoves[p_Event.cookie] = make_relative(l_FullPath);
            return;
          }

          if (p_Event.mask & IN_MOVED_TO) {
            Watcher::Event l_Event;
            l_Event.type = Watcher::EventType::Renamed;
            l_Event.path = make_relative(l_FullPath);

            {
              std::lock_guard<std::mutex> lock(mtx);
              auto l_It = pendingMoves.find(p_Event.cookie);
              if (l_It != pendingMoves.end()) {
                l_Event.oldPath = l_It->second;
                pendingMoves.erase(l_It);
              } else {
                l_Event.type = Watcher::EventType::Added;
              }
            }

            p_Events.push_back(std::move(l_Event));
            return;
          }

          Watcher::Event l_Event;
          l_Event.type = map_mask(p_Event.mask);
          l_Event.path = make_relative(l_FullPath);
          p_Events.push_back(std::move(l_Event));
        }
      };

      Watcher::Watcher() : impl_(std::make_unique<Impl>())
      {
      }

      Watcher::~Watcher()
      {
        stop();
      }

      bool Watcher::start(const std::filesystem::path &p_Root)
      {
        return impl_->start(p_Root);
      }

      void Watcher::stop()
      {
        if (impl_) {
          impl_->stop();
        }
      }

      List<Watcher::Event> Watcher::poll()
      {
        if (!impl_) {
          return {};
        }
        return impl_->poll();
      }

      bool Watcher::running() const noexcept
      {
        return impl_ && impl_->isRunning;
      }

    } // namespace FileSystem
  } // namespace Util
} // namespace Low

#endif // __linux__
