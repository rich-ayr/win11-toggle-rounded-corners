#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <format>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

// clang-format off
#include <windows.h>
#include <tlhelp32.h>
// clang-format on

using namespace std::string_view_literals;

struct Option {
   std::string_view name{};
   std::string_view desc{};
   bool value{};
};

namespace ayr::detail {
static constexpr Option help_option{"help"sv, "Shows all available options."sv};
static bool verbose_enabled{};
} // namespace ayr::detail

inline void set_verbose(bool enabled) noexcept {
   ayr::detail::verbose_enabled = enabled;
}

[[nodiscard]] inline bool get_verbose() noexcept {
   return ayr::detail::verbose_enabled;
}

template <typename... Args>
inline void verbose(std::format_string<Args...> fmt, Args &&...args) {
   if (get_verbose())
      std::print(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
[[noreturn]] inline void error(std::format_string<Args...> fmt, Args &&...args) {
   throw std::runtime_error(std::format(fmt, std::forward<Args>(args)...));
}

template <std::size_t N>
struct ProgramOptions {
   ProgramOptions(int argc, char **argv, auto &&...opts)
      : argc{argc},
        argv{argv},
        command{argv[0]},
        program_name{command.substr(command.find_last_of("/\\") + 1)},
        options{std::array{ayr::detail::help_option, std::forward<decltype(opts)>(opts)...}} {
      constexpr auto trim_and_wrap = [](std::string_view str) constexpr noexcept {
         constexpr auto is_dash = [](auto const c) constexpr noexcept { return c == '-'; };
         return static_cast<std::string_view>(str | std::views::drop_while(is_dash));
      };

      for (auto const arg :
           std::span{argv + 1, argv + argc} | std::views::transform(trim_and_wrap)) {
         for (auto &option : options) {
            if (option.name == arg)
               option.value = true;
         }
      }

      constexpr std::string_view program_name_friendly{"Win11 Toggle Rounded Corners"sv};
      constexpr std::string_view version{"v1.3"sv};
      constexpr std::string_view license{"MIT License"};
      constexpr std::string_view copyright_year{"2026"sv};
      constexpr std::string_view author{"Rich Ayr <rich-ayr@img.ws>"sv};

      std::print("{} {}\nCopyright (C) {} {}, {}\n\n", program_name_friendly, version,
                 copyright_year, author, license);

      if (std::as_const(*this)["help"sv].value)
         print_help();
   }

   template <typename Self>
   [[nodiscard]] constexpr auto get(this Self &&self, std::string_view name) noexcept {
      auto const result =
            std::ranges::find_if(self.options, [name](auto const &option) constexpr noexcept {
               return option.name == name;
            });
      return result != std::end(self.options) ? &*result : nullptr;
   }

   template <typename Self>
   constexpr decltype(auto) operator[](this Self &&self, std::string_view name) {
      auto *option = self.get(name);
      if (!option)
         error("Unknown option '{}'.", name);
      return *option;
   }

   void print_help() const {
      std::print("{} [options]\nOptions:\n"sv, program_name);
      for (auto const &option : options)
         std::print("  --{: <20}: {}\n"sv, option.name, option.desc);
      std::println("");
   }

   int argc;
   char *const *argv;
   std::string_view command;
   std::string_view program_name;
   std::array<Option, N> options;
};

template <typename... Options>
ProgramOptions(int, char **, Options...) -> ProgramOptions<sizeof...(Options) + 1>;

[[nodiscard]] IMAGE_NT_HEADERS64 const *image_nt_headers(void const *base) noexcept {
   if (!base)
      return nullptr;

   auto const dos_header = static_cast<IMAGE_DOS_HEADER const *>(base);
   if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
      return nullptr;

   auto const nt_headers = reinterpret_cast<IMAGE_NT_HEADERS64 const *>(
         static_cast<std::uint8_t const *>(base) + dos_header->e_lfanew);
   if (nt_headers->Signature != IMAGE_NT_SIGNATURE)
      return nullptr;

   return nt_headers;
}

template <typename T = uint8_t>
[[nodiscard]] std::span<T const> get_section(void const *base, std::string_view name) noexcept {
   auto const nt_hdrs = image_nt_headers(base);
   if (!nt_hdrs)
      return {};

   for (auto sec_header = IMAGE_FIRST_SECTION(nt_hdrs);
        sec_header < IMAGE_FIRST_SECTION(nt_hdrs) + nt_hdrs->FileHeader.NumberOfSections;
        sec_header++) {
      if (name == reinterpret_cast<char const *>(sec_header->Name)) {
         return {reinterpret_cast<T const *>(static_cast<uint8_t const *>(base)
                                             + sec_header->VirtualAddress),
                 sec_header->Misc.VirtualSize / sizeof(T)};
      }
   }
   return {};
}

[[nodiscard]] std::optional<uint64_t> find_module_base(DWORD pid,
                                                       std::string_view module_name) noexcept {
   auto const snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
   if (snapshot == INVALID_HANDLE_VALUE)
      return {};

   auto entry = MODULEENTRY32{};
   entry.dwSize = sizeof(MODULEENTRY32);

   if (Module32First(snapshot, &entry)) {
      do {
         if (module_name == entry.szModule) {
            CloseHandle(snapshot);
            return reinterpret_cast<uint64_t>(entry.modBaseAddr);
         }
      } while (Module32Next(snapshot, &entry));
   }

   CloseHandle(snapshot);
   return {};
}

[[nodiscard]] std::string last_error_message(DWORD code = GetLastError()) {
   LPSTR buffer = nullptr;
   auto const length = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                                            | FORMAT_MESSAGE_IGNORE_INSERTS,
                                      nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                      reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

   if (!length)
      return std::format("unknown error ({:#x})", code);

   std::string message{buffer, length};
   LocalFree(buffer);

   message.erase(message.find_last_not_of("\r\n") + 1);

   return std::format("{} ({:#x})", message, code);
}

[[nodiscard]] bool enable_privilege(LPCTSTR name) noexcept {
   TOKEN_PRIVILEGES privilege{};
   privilege.PrivilegeCount = 1;
   privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

   if (!LookupPrivilegeValue(nullptr, name, &privilege.Privileges[0].Luid))
      return false;

   HANDLE token{};
   if (!OpenProcessToken(reinterpret_cast<HANDLE>(-1), TOKEN_ADJUST_PRIVILEGES, &token))
      return false;

   if (!AdjustTokenPrivileges(token, FALSE, &privilege, sizeof(privilege), nullptr, nullptr)) {
      CloseHandle(token);
      return false;
   }

   CloseHandle(token);
   return true;
}

int main(int argc, char **argv) try {
   using namespace std::chrono_literals;
   using clock = std::chrono::steady_clock;

   ProgramOptions const options{
         argc,
         argv,
         Option{"verbose"sv, "Enables verbose output."sv},
         Option{"disable"sv,
                "Always disables rounded corners. Has precedence over --enable and --small."sv},
         Option{"enable"sv, "Always enables rounded corners."sv},
         Option{"small"sv, "Enables small rounded corners. Has precedence over --enable."sv}};

   set_verbose(options["verbose"sv].value);

   auto should_disable = options["disable"sv].value;
   auto const should_small = options["small"sv].value;
   auto const should_override_toggle = should_disable || options["enable"sv].value || should_small;

   if (!enable_privilege(SE_DEBUG_NAME))
      error("Failed to enable '{}', make sure you are running as admin.", SE_DEBUG_NAME);

   DWORD dwm_pid{};

   for (auto const deadline = clock::now() + 5s; clock::now() < deadline;) {
      if (auto const dwm_hwnd = FindWindowA("Dwm", nullptr))
         if (GetWindowThreadProcessId(dwm_hwnd, &dwm_pid))
            break;
      std::this_thread::sleep_for(250ms);
   }

   if (!dwm_pid)
      error("Failed to find dwm process.\n");

   verbose("Found dwm.exe process, pid: {}.\n", dwm_pid);

   auto const dwm_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwm_pid);
   if (!dwm_process)
      error("Failed to open dwm.exe process: {}", last_error_message());

   verbose("Opened process handle {:#x} to dwm.exe.\n", reinterpret_cast<uint64_t>(dwm_process));

   auto const udwm_base = find_module_base(dwm_pid, std::string_view{"udwm.dll"});
   if (!udwm_base)
      error("Failed to find udwm.dll module inside dwm.exe process!");

   verbose("Found udwm.dll mapped at {:#x}.\n", udwm_base.value());

   auto const udwm_dll = LoadLibraryExA("udwm.dll", nullptr, DONT_RESOLVE_DLL_REFERENCES);
   if (!udwm_dll)
      error("Failed to load udwm.dll locally: {}", last_error_message());

   auto const patch_targets =
         get_section<float>(udwm_dll, ".rdata")
         | std::views::filter([](auto const &flt) { return flt == 4.f || flt == 8.f; })
         | std::views::transform([=](float const &flt) {
              auto rva = reinterpret_cast<uint8_t const *>(&flt)
                         - reinterpret_cast<uint8_t const *>(udwm_dll);
              auto rebased = reinterpret_cast<float *>(udwm_base.value() + rva);
              return std::make_tuple(flt, rebased);
           })
         | std::ranges::to<std::vector>();

   if (patch_targets.empty())
      error("Found no rounding-radius constants to patch in udwm.dll's .rdata section — layout may "
            "have changed.");

   for (auto const &[original, ptr] : patch_targets) {
      constexpr auto kNearZeroRadius = 0.001f;
      constexpr auto kSmallRadius = 4.0f;
      float value{};
      SIZE_T out_size{};

      if (!ReadProcessMemory(dwm_process, ptr, &value, sizeof(float), &out_size)
          || out_size != sizeof(float))
         error("Failed to read rounding float from dwm.exe: {}", last_error_message());

      auto const is_disabled = value == kNearZeroRadius;
      if (!should_override_toggle)
         should_disable = !is_disabled;

      auto const new_border_radius = [&] {
         if (should_disable)
            return kNearZeroRadius;
         if (should_small)
            return kSmallRadius;
         return original;
      }();
      verbose("Writing {} to border radius {:#x}\n", new_border_radius,
              reinterpret_cast<uint64_t>(ptr));

      DWORD old_protect{};
      if (!VirtualProtectEx(dwm_process, ptr, sizeof(float), PAGE_READWRITE, &old_protect))
         error("Failed to unprotect memory: {}", last_error_message());

      out_size = {};
      if (!WriteProcessMemory(dwm_process, ptr, &new_border_radius, sizeof(float), &out_size)
          || out_size != sizeof(float))
         error("Failed to write new border radius to dwm.exe: {}", last_error_message());

      if (!VirtualProtectEx(dwm_process, ptr, sizeof(float), old_protect, &old_protect))
         error("Failed to protect memory: {}", last_error_message());
   }

   if (should_disable)
      std::print("Your Windows 11 experience is now enhanced!\n");
   else
      std::print("Your Windows 11 experience is now dehanced!\n");

   return 0;
} catch (std::exception const &e) {
   std::print(stderr, "{}\n", e.what());
   return 1;
}
