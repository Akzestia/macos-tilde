#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <vector>

struct KeyMapping {
  std::string character;
  bool requireShift = false;
  bool requireControl = false;
  bool requireCommand = false;
  bool requireOption = false;
};

bool debugMode = false;
std::map<CGKeyCode, std::vector<KeyMapping>> keyMappings;

bool fileExists(const std::string &path) {
  struct stat buffer;
  return (stat(path.c_str(), &buffer) == 0);
}

bool createDirectoryRecursive(const std::string &path) {
  size_t pos = 0;
  std::string dir;

  while ((pos = path.find('/', pos + 1)) != std::string::npos) {
    dir = path.substr(0, pos);
    struct stat st;
    if (stat(dir.c_str(), &st) != 0) {
      if (mkdir(dir.c_str(), 0755) != 0) {
        return false;
      }
    }
  }
  return true;
}

bool createDefaultConfig(const std::string &filepath) {
  size_t lastSlash = filepath.find_last_of('/');
  if (lastSlash != std::string::npos) {
    std::string dirPath = filepath.substr(0, lastSlash);
    if (!createDirectoryRecursive(dirPath)) {
      fprintf(stderr, "Failed to create directory: %s\n", dirPath.c_str());
      return false;
    }
  }

  std::ofstream file(filepath);
  if (!file.is_open()) {
    fprintf(stderr, "Failed to create config file: %s\n", filepath.c_str());
    return false;
  }

  file << "{\n";
  file << "  // Example configuration:\n";
  file << "  // \"keycode\": [\"modifiers\", \"character\"]\n";
  file << "  // Available modifiers: shift, control/ctrl, command/cmd, "
          "option/alt\n";
  file << "  // Combine with + : \"shift+command\"\n";
  file << "  \n";
  file << "  \"10\": [\"shift\", \"~\"],\n";
  file << "  \"10\": [\"\", \"`\"],\n";
  file << "}\n";

  file.close();
  printf("Created default config file: %s\n", filepath.c_str());
  return true;
}

bool loadConfig(const std::string &filename) {
  if (!fileExists(filename)) {
    printf("Config file not found, creating default: %s\n", filename.c_str());
    if (!createDefaultConfig(filename)) {
      return false;
    }
  }

  std::ifstream file(filename);
  if (!file.is_open()) {
    fprintf(stderr, "Could not open config file: %s\n", filename.c_str());
    return false;
  }

  std::string line;
  CGKeyCode currentKey = 0;

  while (std::getline(file, line)) {
    line.erase(0, line.find_first_not_of(" \t\n\r"));
    line.erase(line.find_last_not_of(" \t\n\r") + 1);

    if (line.empty() || line[0] == '/' || line[0] == '{' || line[0] == '}') {
      continue;
    }

    if (line.find("\"") != std::string::npos) {
      size_t firstQuote = line.find("\"");
      size_t secondQuote = line.find("\"", firstQuote + 1);
      if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
        std::string keycodeStr =
            line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        currentKey = std::stoi(keycodeStr);

        size_t bracketStart = line.find("[");
        size_t bracketEnd = line.find("]");

        if (bracketStart != std::string::npos &&
            bracketEnd != std::string::npos) {
          std::string content =
              line.substr(bracketStart + 1, bracketEnd - bracketStart - 1);

          KeyMapping mapping;

          size_t comma = content.find(",");
          if (comma != std::string::npos) {
            std::string modifiers = content.substr(0, comma);
            modifiers.erase(0, modifiers.find_first_not_of(" \t\""));
            modifiers.erase(modifiers.find_last_not_of(" \t\"") + 1);

            if (modifiers.find("shift") != std::string::npos)
              mapping.requireShift = true;
            if (modifiers.find("control") != std::string::npos ||
                modifiers.find("ctrl") != std::string::npos)
              mapping.requireControl = true;
            if (modifiers.find("command") != std::string::npos ||
                modifiers.find("cmd") != std::string::npos)
              mapping.requireCommand = true;
            if (modifiers.find("option") != std::string::npos ||
                modifiers.find("alt") != std::string::npos)
              mapping.requireOption = true;

            std::string character = content.substr(comma + 1);
            character.erase(0, character.find_first_not_of(" \t\""));
            character.erase(character.find_last_not_of(" \t\"") + 1);
            mapping.character = character;

            keyMappings[currentKey].push_back(mapping);
          } else {
            content.erase(0, content.find_first_not_of(" \t\""));
            content.erase(content.find_last_not_of(" \t\"") + 1);
            mapping.character = content;
            keyMappings[currentKey].push_back(mapping);
          }
        }
      }
    }
  }

  file.close();
  printf("Loaded %zu key mappings from config\n", keyMappings.size());
  return true;
}

void printModifiers(CGEventFlags flags) {
  printf(" [");
  if (flags & kCGEventFlagMaskShift)
    printf(" Shift ");
  if (flags & kCGEventFlagMaskControl)
    printf(" Control ");
  if (flags & kCGEventFlagMaskCommand)
    printf(" Command ");
  if (flags & kCGEventFlagMaskAlternate)
    printf(" Option ");
  if (flags & kCGEventFlagMaskAlphaShift)
    printf(" CapsLock ");
  printf("]");
}

CGEventRef eventTapCallback(CGEventTapProxy proxy, CGEventType type,
                            CGEventRef event, void *refcon) {
  if (type == kCGEventKeyDown) {
    CGKeyCode keycode =
        (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    CGEventFlags flags = CGEventGetFlags(event);

    bool shiftPressed = (flags & kCGEventFlagMaskShift) != 0;
    bool controlPressed = (flags & kCGEventFlagMaskControl) != 0;
    bool commandPressed = (flags & kCGEventFlagMaskCommand) != 0;
    bool optionPressed = (flags & kCGEventFlagMaskAlternate) != 0;

    if (debugMode) {
      printf("Keycode: %d x", keycode);
      printModifiers(flags);
      printf("\n");
      return event;
    }

    auto it = keyMappings.find(keycode);
    if (it != keyMappings.end()) {
      for (const auto &mapping : it->second) {
        bool modifiersMatch = true;

        if (mapping.requireShift && !shiftPressed)
          modifiersMatch = false;
        if (mapping.requireControl && !controlPressed)
          modifiersMatch = false;
        if (mapping.requireCommand && !commandPressed)
          modifiersMatch = false;
        if (mapping.requireOption && !optionPressed)
          modifiersMatch = false;

        std::cout << modifiersMatch << '\n';
        if (modifiersMatch && !mapping.character.empty()) {
          std::u16string u16str;
          for (char c : mapping.character) {
            u16str.push_back(static_cast<UniChar>(c));
          }

          CGEventKeyboardSetUnicodeString(
              event, u16str.length(),
              reinterpret_cast<const UniChar *>(u16str.data()));

          return event;
        }
      }
    }
  }
  return event;
}

int main(int argc, char *argv[]) {
  bool daemonMode = false;
  const char *home = getenv("HOME");

  if (!home) {
    fprintf(stderr, "Error: HOME environment variable not set\n");
    return 1;
  }

  std::string configFile =
      std::string(home) + "/.config/macos-tilda/config.json";

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--keycodes") == 0) {
      debugMode = true;
    } else if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
      configFile = argv[++i];
    }
  }

  if (!debugMode) {
    if (!loadConfig(configFile)) {
      fprintf(stderr, "Warning: Could not load config file, exiting\n");
      return 1;
    }
  } else {
    printf("Debug mode: Press any key to see its keycode and modifiers\n");
  }

  CFMachPortRef eventTap;
  CGEventMask eventMask;
  CFRunLoopSourceRef runLoopSource;

  eventMask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp);

  eventTap = CGEventTapCreate(kCGHIDEventTap, kCGHeadInsertEventTap,
                              kCGEventTapOptionDefault, eventMask,
                              eventTapCallback, NULL);

  if (!eventTap) {
    fprintf(stderr, "Failed to create event tap.\n");
    fprintf(stderr, "Enable Accessibility permissions.\n");
    return 1;
  }

  runLoopSource =
      CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource,
                     kCFRunLoopCommonModes);

  CGEventTapEnable(eventTap, true);

  printf("Event tap running. Press Ctrl+C to exit.\n");

  CFRunLoopRun();

  CFRelease(eventTap);
  CFRelease(runLoopSource);

  return 0;
}
