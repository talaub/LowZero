#include "LowCliEvents.h"

#include <nlohmann/json.hpp>

namespace Low {
  namespace Cli {
    namespace Events {
      using Json = nlohmann::json;

      static const char *g_TypeNames[] = {"section",     "log",
                                          "task_begin",  "task_update",
                                          "task_end",    "result"};
      static const char *g_LevelNames[] = {"debug", "detail",  "info",
                                           "success", "warn", "error"};
      static const char *g_StatusNames[] = {"ok", "failed",
                                            "cancelled"};

      template <size_t N>
      static int find_name(const char *(&p_Names)[N],
                           const std::string &p_Name)
      {
        for (size_t i = 0; i < N; ++i) {
          if (p_Name == p_Names[i]) {
            return static_cast<int>(i);
          }
        }
        return -1;
      }

      static Util::String get_string(const Json &p_Object,
                                     const char *p_Key)
      {
        auto i_It = p_Object.find(p_Key);
        if (i_It == p_Object.end() || !i_It->is_string()) {
          return "";
        }
        return i_It->get<std::string>().c_str();
      }

      static int64_t get_int(const Json &p_Object, const char *p_Key)
      {
        auto i_It = p_Object.find(p_Key);
        if (i_It == p_Object.end() || !i_It->is_number()) {
          return -1;
        }
        return i_It->get<int64_t>();
      }

      bool parse(Util::StringView p_Line, Event &p_Event)
      {
        // Cheap reject for plain text lines before invoking the parser
        size_t l_Start = 0;
        while (l_Start < p_Line.size() &&
               (p_Line[l_Start] == ' ' || p_Line[l_Start] == '\t')) {
          ++l_Start;
        }
        if (l_Start >= p_Line.size() || p_Line[l_Start] != '{') {
          return false;
        }

        const Json l_Object = Json::parse(
            p_Line.begin(), p_Line.end(), nullptr, false);
        if (l_Object.is_discarded() || !l_Object.is_object()) {
          return false;
        }

        const int l_Type = find_name(
            g_TypeNames, get_string(l_Object, "type").c_str());
        if (l_Type < 0) {
          return false;
        }

        p_Event = Event();
        p_Event.type = static_cast<EventType>(l_Type);
        p_Event.id = get_string(l_Object, "id");
        p_Event.label = get_string(l_Object, "label");
        p_Event.message = get_string(l_Object, "msg");
        p_Event.hint = get_string(l_Object, "hint");
        p_Event.done = get_int(l_Object, "done");
        p_Event.total = get_int(l_Object, "total");

        const int l_Level = find_name(
            g_LevelNames, get_string(l_Object, "level").c_str());
        if (l_Level >= 0) {
          p_Event.level = static_cast<LogLevel>(l_Level);
        }

        const int l_Status = find_name(
            g_StatusNames, get_string(l_Object, "status").c_str());
        if (l_Status >= 0) {
          p_Event.status = static_cast<TaskStatus>(l_Status);
        }

        auto l_Details = l_Object.find("details");
        if (l_Details != l_Object.end() && l_Details->is_array()) {
          for (const Json &i_Detail : *l_Details) {
            if (i_Detail.is_string()) {
              p_Event.details.push_back(
                  i_Detail.get<std::string>().c_str());
            }
          }
        }

        return true;
      }

      Util::String to_json(const Event &p_Event)
      {
        Json l_Object = Json::object();
        l_Object["type"] = g_TypeNames[static_cast<int>(p_Event.type)];

        switch (p_Event.type) {
        case EventType::SECTION:
          l_Object["label"] = p_Event.label.c_str();
          break;
        case EventType::LOG:
          l_Object["level"] = g_LevelNames[static_cast<int>(p_Event.level)];
          l_Object["msg"] = p_Event.message.c_str();
          if (!p_Event.details.empty()) {
            Json l_Details = Json::array();
            for (const Util::String &i_Detail : p_Event.details) {
              l_Details.push_back(i_Detail.c_str());
            }
            l_Object["details"] = l_Details;
          }
          if (!p_Event.hint.empty()) {
            l_Object["hint"] = p_Event.hint.c_str();
          }
          break;
        case EventType::TASK_BEGIN:
          l_Object["id"] = p_Event.id.c_str();
          l_Object["label"] = p_Event.label.c_str();
          if (p_Event.total >= 0) {
            l_Object["total"] = p_Event.total;
          }
          break;
        case EventType::TASK_UPDATE:
          l_Object["id"] = p_Event.id.c_str();
          if (p_Event.done >= 0) {
            l_Object["done"] = p_Event.done;
          }
          if (!p_Event.label.empty()) {
            l_Object["label"] = p_Event.label.c_str();
          }
          break;
        case EventType::TASK_END:
          l_Object["id"] = p_Event.id.c_str();
          l_Object["status"] =
              g_StatusNames[static_cast<int>(p_Event.status)];
          break;
        case EventType::RESULT:
          l_Object["msg"] = p_Event.message.c_str();
          break;
        }

        // Child output is not guaranteed to be valid UTF-8
        return l_Object
            .dump(-1, ' ', false, Json::error_handler_t::replace)
            .c_str();
      }
    } // namespace Events
  } // namespace Cli
} // namespace Low
