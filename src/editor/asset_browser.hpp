#pragma once

#include <filesystem>

#include "dui/dui.hpp"
#include "editor/state.hpp"
#include "string.hpp"

namespace Editor
{
struct AssetBrowser {
  StaticString<1024> current_directory;
};

void do_asset_browser(Editor::State *state, AssetBrowser *browser)
{
  Dui::start_window("asset_browser", {100, 100, 200, 200});

  std::filesystem::path root =
      std::string_view((char *)state->resource_path.data, state->resource_path.size);
  std::filesystem::path current_directory = std::string_view(
      (char *)browser->current_directory.data, browser->current_directory.size);

  if (browser->current_directory.size > 0) {
    String go_up_one = "../";
    DuiId id         = Dui::hash(go_up_one);
    Dui::directory_item(id, go_up_one, false, false);
    if (Dui::clicked(id)) {
      u32 last_slash_index = browser->current_directory.size - 1;
      while (browser->current_directory[last_slash_index] != '/' &&
             last_slash_index > 0)
        last_slash_index--;
      browser->current_directory.size = last_slash_index;
    }
  }

  for (const auto &entry :
       std::filesystem::directory_iterator(root / current_directory)) {
    std::string relative_path =
        std::filesystem::relative(entry.path(), root).string();
    std::replace(relative_path.begin(), relative_path.end(), '\\', '/');

    b8 is_directory = entry.is_directory();

    String path = {(u8 *)relative_path.c_str(), (u32)relative_path.size()};
    DuiId id    = Dui::hash(path);
    if (Dui::directory_item(id, path, false, false)) {
      Dui::button("inside tree view", {100, 40}, {1, 0, 1, 1});
      Dui::next_line();
    }

    if (Dui::clicked(id)) {
      if (is_directory) {
        memcpy(browser->current_directory.data, relative_path.c_str(),
               relative_path.size());
        browser->current_directory.size = relative_path.size();
      } else {
        state->material_editor_windows.push_back({path});
      }
    }
  }

  Dui::end_window();
}
}  // namespace Editor