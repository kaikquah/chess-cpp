#include "uci/options.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace chessbot::uci {

namespace {

std::string trim(std::string_view view) {
  std::size_t start = 0;
  std::size_t end = view.size();
  while (start < end && std::isspace(static_cast<unsigned char>(view[start]))) {
    ++start;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(view[end - 1]))) {
    --end;
  }
  return std::string(view.substr(start, end - start));
}

bool parse_bool(std::string_view value, bool& out) {
  std::string lowered;
  lowered.reserve(value.size());
  for (char ch : value) {
    lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  if (lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on") {
    out = true;
    return true;
  }
  if (lowered == "false" || lowered == "0" || lowered == "no" || lowered == "off") {
    out = false;
    return true;
  }
  return false;
}

bool parse_int(std::string_view value, int& out) {
  const char* begin = value.data();
  const char* end = begin + value.size();
  auto result = std::from_chars(begin, end, out);
  return result.ec == std::errc{} && result.ptr == end;
}

}  // namespace

OptionRegistry::OptionRegistry(std::filesystem::path state_path)
    : state_path_(std::move(state_path)) {
  if (state_path_.empty()) {
    state_path_ = std::filesystem::path("options_state.ini");
  }
}

void OptionRegistry::register_check(std::string name, bool default_value) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  OptionEntry entry{};
  entry.meta.name = std::move(name);
  entry.meta.type = OptionType::kCheck;
  entry.meta.default_value = default_value ? "true" : "false";
  entry.value = entry.meta.default_value;
  entries_[canonical] = std::move(entry);
}

void OptionRegistry::register_spin(std::string name, int default_value, int min_value, int max_value) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  OptionEntry entry{};
  entry.meta.name = std::move(name);
  entry.meta.type = OptionType::kSpin;
  entry.meta.default_value = std::to_string(default_value);
  entry.meta.min = min_value;
  entry.meta.max = max_value;
  entry.value = entry.meta.default_value;
  entries_[canonical] = std::move(entry);
}

void OptionRegistry::register_string(std::string name, std::string default_value) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  OptionEntry entry{};
  entry.meta.name = std::move(name);
  entry.meta.type = OptionType::kString;
  entry.meta.default_value = std::move(default_value);
  entry.value = entry.meta.default_value;
  entries_[canonical] = std::move(entry);
}

void OptionRegistry::register_combo(std::string name, std::string default_value,
                                    std::vector<std::string> allowed_values) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  OptionEntry entry{};
  entry.meta.name = std::move(name);
  entry.meta.type = OptionType::kCombo;
  entry.meta.default_value = std::move(default_value);
  entry.meta.combo_values = std::move(allowed_values);
  entry.value = entry.meta.default_value;
  entries_[canonical] = std::move(entry);
}

bool OptionRegistry::has_option(std::string_view name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  return entries_.find(canonical) != entries_.end();
}

bool OptionRegistry::set_option(std::string_view name, std::string value) {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  auto it = entries_.find(canonical);
  if (it == entries_.end()) {
    return false;
  }
  OptionEntry& entry = it->second;
  if (!validate_value(entry, value)) {
    return false;
  }
  if (entry.value != value) {
    entry.value = std::move(value);
    dirty_ = true;
  }
  return true;
}

bool OptionRegistry::get_bool(std::string_view name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  auto it = entries_.find(canonical);
  if (it == entries_.end()) {
    return false;
  }
  bool val = false;
  if (parse_bool(it->second.value, val)) {
    return val;
  }
  return false;
}

int OptionRegistry::get_int(std::string_view name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  auto it = entries_.find(canonical);
  if (it == entries_.end()) {
    return 0;
  }
  int out = 0;
  if (parse_int(it->second.value, out)) {
    return out;
  }
  parse_int(it->second.meta.default_value, out);
  return out;
}

std::string OptionRegistry::get_string(std::string_view name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  auto it = entries_.find(canonical);
  if (it == entries_.end()) {
    return {};
  }
  return it->second.value;
}

std::optional<OptionMetadata> OptionRegistry::find_option(std::string_view name) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const std::string canonical = normalize_name(name);
  auto it = entries_.find(canonical);
  if (it == entries_.end()) {
    return std::nullopt;
  }
  return it->second.meta;
}

std::vector<OptionMetadata> OptionRegistry::list_options() const {
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<OptionMetadata> out;
  out.reserve(entries_.size());
  for (const auto& [canonical, entry] : entries_) {
    (void)canonical;
    out.push_back(entry.meta);
  }
  std::sort(out.begin(), out.end(), [](const OptionMetadata& lhs, const OptionMetadata& rhs) {
    return lhs.name < rhs.name;
  });
  return out;
}

std::string OptionRegistry::normalize_name(std::string_view name) {
  std::string normalized;
  normalized.reserve(name.size());
  for (char ch : name) {
    if (std::isspace(static_cast<unsigned char>(ch))) {
      continue;
    }
    normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
  }
  return normalized;
}

bool OptionRegistry::validate_value(const OptionEntry& entry, const std::string& value) const {
  switch (entry.meta.type) {
    case OptionType::kCheck: {
      bool dummy = false;
      return parse_bool(value, dummy);
    }
    case OptionType::kSpin: {
      int parsed = 0;
      if (!parse_int(value, parsed)) {
        return false;
      }
      return parsed >= entry.meta.min && parsed <= entry.meta.max;
    }
    case OptionType::kString:
      return true;
    case OptionType::kCombo: {
      return std::find(entry.meta.combo_values.begin(), entry.meta.combo_values.end(), value) !=
             entry.meta.combo_values.end();
    }
  }
  return false;
}

void OptionRegistry::load_persistent_values() {
  std::lock_guard<std::mutex> lock(mutex_);
  load_from_disk_locked();
}

void OptionRegistry::persist() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!dirty_) {
    return;
  }
  save_to_disk_locked();
  dirty_ = false;
}

void OptionRegistry::load_from_disk_locked() {
  if (!std::filesystem::exists(state_path_)) {
    return;
  }
  std::ifstream file(state_path_);
  if (!file.is_open()) {
    return;
  }
  std::string line;
  while (std::getline(file, line)) {
    const std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#') {
      continue;
    }
    const auto eq_pos = trimmed.find('=');
    if (eq_pos == std::string::npos) {
      continue;
    }
    const std::string key = trim(trimmed.substr(0, eq_pos));
    const std::string value = trim(trimmed.substr(eq_pos + 1));
    const std::string canonical = normalize_name(key);
    auto it = entries_.find(canonical);
    if (it == entries_.end()) {
      continue;
    }
    if (validate_value(it->second, value)) {
      it->second.value = value;
    }
  }
}

void OptionRegistry::save_to_disk_locked() const {
  if (state_path_.empty()) {
    return;
  }
  std::ofstream file(state_path_, std::ios::trunc);
  if (!file.is_open()) {
    return;
  }
  file << "# chessbot uci option state\n";
  for (const auto& [canonical, entry] : entries_) {
    (void)canonical;
    file << entry.meta.name << '=' << entry.value << '\n';
  }
}

void register_default_options(OptionRegistry& registry) {
  registry.register_spin("Hash", 16, 1, 4096);
  registry.register_spin("Threads", 1, 1, 1);
  registry.register_check("Ponder", false);
  registry.register_check("Debug Log", false);
  registry.register_check("Eval Log", false);
  registry.register_spin("EvalTempoBonus", 10, -50, 50);
  registry.register_spin("TimeSafetyMargin", 20, 0, 1000);
  registry.register_spin("MoveOverhead", 15, 0, 1000);
  registry.register_spin("TimeReservePercent", 8, 0, 50);
}

Engine::Options derive_engine_options(const OptionRegistry& registry, const Engine::Options& base) {
  Engine::Options opts = base;
  opts.hash_mb = std::clamp(registry.get_int("Hash"), 1, 4096);
  opts.eval_tempo_bonus = registry.get_int("EvalTempoBonus");
  opts.ponder_enabled = registry.get_bool("Ponder");
  opts.debug_logging = registry.get_bool("Debug Log");
  opts.eval_logging = registry.get_bool("Eval Log");
  opts.time_safety_margin_ms = std::clamp(registry.get_int("TimeSafetyMargin"), 0, 5000);
  opts.move_overhead_ms = std::clamp(registry.get_int("MoveOverhead"), 0, 5000);
  opts.time_reserve_percent = std::clamp(registry.get_int("TimeReservePercent"), 0, 50);
  return opts;
}

TimeManagerConfig derive_time_manager_config(const OptionRegistry& registry,
                                             const Engine::Options& engine_options) {
  (void)registry;
  TimeManagerConfig config;
  config.safety_margin_ms = engine_options.time_safety_margin_ms;
  config.move_overhead_ms = engine_options.move_overhead_ms;
  config.reserve_ratio = std::clamp(static_cast<double>(engine_options.time_reserve_percent) / 100.0, 0.0, 0.5);
  config.minimum_reserve_ms = 50;
  config.minimum_allocation_ms = 10;
  return config;
}

}  // namespace chessbot::uci
