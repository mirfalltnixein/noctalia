#include "shell/settings/font_family_catalog.h"

#include "core/process/process.h"
#include "i18n/i18n.h"
#include "util/string_utils.h"

#include <algorithm>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace settings {
  namespace {

    std::vector<std::string> discoverFontFamiliesUncached() {
      if (!process::commandExists("fc-list")) {
        return {};
      }

      const auto result = process::runSync({"fc-list", ":", "family"});
      if (!result) {
        return {};
      }

      return parseFontFamilyList(result.out);
    }

  } // namespace

  std::vector<std::string> parseFontFamilyList(std::string_view output) {
    const auto unescape = [](std::string_view value) {
      std::string result;
      result.reserve(value.size());
      for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
          ++i;
        }
        result += value[i];
      }
      return result;
    };

    std::unordered_set<std::string> seen;
    seen.reserve(4096);

    std::size_t lineStart = 0;
    while (lineStart <= output.size()) {
      const std::size_t lineEnd = output.find('\n', lineStart);
      const std::string_view line =
          lineEnd == std::string::npos ? output.substr(lineStart) : output.substr(lineStart, lineEnd - lineStart);

      std::size_t tokenStart = 0;
      while (tokenStart <= line.size()) {
        const std::size_t tokenEnd = line.find(',', tokenStart);
        const std::string_view token =
            tokenEnd == std::string::npos ? line.substr(tokenStart) : line.substr(tokenStart, tokenEnd - tokenStart);
        std::string family = unescape(StringUtils::trim(token));
        if (!family.empty()) {
          seen.insert(std::move(family));
        }
        if (tokenEnd == std::string::npos) {
          break;
        }
        tokenStart = tokenEnd + 1;
      }

      if (lineEnd == std::string::npos) {
        break;
      }
      lineStart = lineEnd + 1;
    }

    std::vector<std::string> families(seen.begin(), seen.end());
    std::ranges::sort(families, [](const std::string& a, const std::string& b) {
      return StringUtils::toLower(a) < StringUtils::toLower(b);
    });
    return families;
  }

  const std::vector<std::string>& discoverFontFamilies() {
    static const std::vector<std::string> kFamilies = discoverFontFamiliesUncached();
    return kFamilies;
  }

  std::vector<WidgetSettingSelectOption> buildFontFamilySelectOptions() {
    const std::vector<std::string>& families = discoverFontFamilies();

    std::vector<WidgetSettingSelectOption> options;
    options.reserve(families.size() + 1);
    options.push_back(WidgetSettingSelectOption{"", i18n::tr("desktop-widgets.editor.settings.font-family-default")});
    for (const std::string& family : families) {
      options.push_back(WidgetSettingSelectOption{family, family});
    }
    return options;
  }

} // namespace settings
