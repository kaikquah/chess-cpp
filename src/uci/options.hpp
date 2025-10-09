#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/engine.hpp"
#include "engine/time_manager.hpp"

namespace chessbot::uci {

enum class OptionType {
  kCheck,
  kSpin,
  kString,
  kCombo,
};

struct OptionMetadata {
  std::string name;
  OptionType type;
  std::string default_value;
  int min = 0;
  int max = 0;
  std::vector<std::string> combo_values;
};

class OptionRegistry {
public:
  explicit OptionRegistry(std::filesystem::path state_path = {});

  void register_check(std::string name, bool default_value);
  void register_spin(std::string name, int default_value, int min_value, int max_value);
  void register_string(std::string name, std::string default_value);
  void register_combo(std::string name, std::string default_value, std::vector<std::string> allowed_values);

  [[nodiscard]] bool has_option(std::string_view name) const;
  bool set_option(std::string_view name, std::string value);

  [[nodiscard]] bool get_bool(std::string_view name) const;
  [[nodiscard]] int get_int(std::string_view name) const;
  [[nodiscard]] std::string get_string(std::string_view name) const;
  [[nodiscard]] std::optional<OptionMetadata> find_option(std::string_view name) const;

  [[nodiscard]] std::vector<OptionMetadata> list_options() const;

  void load_persistent_values();
  void persist();

private:
  struct OptionEntry {
    OptionMetadata meta;
    std::string value;
  };

  [[nodiscard]] static std::string normalize_name(std::string_view name);
  [[nodiscard]] bool validate_value(const OptionEntry& entry, const std::string& value) const;
  void load_from_disk_locked();
  void save_to_disk_locked() const;

  std::filesystem::path state_path_;
  mutable std::mutex mutex_;
  std::unordered_map<std::string, OptionEntry> entries_;
  bool dirty_ = false;
};

void register_default_options(OptionRegistry& registry);
Engine::Options derive_engine_options(const OptionRegistry& registry,
                                      const Engine::Options& base = {});
TimeManagerConfig derive_time_manager_config(const OptionRegistry& registry,
                                             const Engine::Options& engine_options);

}  // namespace chessbot::uci
