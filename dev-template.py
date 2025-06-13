import zipfile
import os

# Directory structure
base_dir = "/mnt/data/obs-dynamic-path-plugin"
src_dir = os.path.join(base_dir, "src")
os.makedirs(src_dir, exist_ok=True)

# C++ plugin source code
plugin_cpp = """// obs-dynamic-recording-path-plugin.cpp

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <ctime>
#include <string>
#include <sstream>
#include <filesystem>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-dynamic-path-plugin", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
    return "Dynamic Recording Path Plugin";
}

static std::string selected_source_name;

static std::string get_selected_source_name()
{
    return selected_source_name.empty() ? "UnknownSource" : selected_source_name;
}

static std::string get_date_string()
{
    std::time_t t = std::time(nullptr);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", &tm);
    return buffer;
}

static std::string expand_path_template(const std::string &base_path)
{
    std::string source = get_selected_source_name();
    std::string date = get_date_string();

    std::string suffix = "%DATE%/%SOURCE%";
    size_t pos;
    while ((pos = suffix.find("%SOURCE%")) != std::string::npos)
        suffix.replace(pos, 9, source);
    while ((pos = suffix.find("%DATE%")) != std::string::npos)
        suffix.replace(pos, 7, date);

    std::filesystem::path full_path = std::filesystem::path(base_path) / suffix;
    std::filesystem::create_directories(full_path);
    return full_path.string();
}

static void on_frontend_event(enum obs_frontend_event event)
{
    if (event == OBS_FRONTEND_EVENT_RECORDING_STARTING)
    {
        obs_output_t *output = obs_frontend_get_recording_output();
        if (output)
        {
            const char *current_path = obs_output_get_path(output);
            std::string new_path = expand_path_template(current_path);
            obs_output_set_path(output, new_path.c_str());
            obs_output_release(output);
        }
    }
}

static bool source_filter(void *data, obs_source_t *source)
{
    return obs_source_get_type(source) == OBS_SOURCE_TYPE_INPUT;
}

static void dynamic_path_properties_modified(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
    const char *name = obs_data_get_string(settings, "selected_source");
    if (name)
        selected_source_name = name;
}

static obs_properties_t *dynamic_path_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();

    obs_property_t *p = obs_properties_add_list(props, "selected_source",
        "Source for Folder Naming", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

    obs_property_list_add_sources(p, source_filter, nullptr);
    obs_property_set_modified_callback(p, dynamic_path_properties_modified);

    return props;
}

bool obs_module_load(void)
{
    blog(LOG_INFO, "Dynamic Recording Path Plugin loaded.");
    obs_frontend_add_event_callback(on_frontend_event, nullptr);
    obs_register_module_config_path("obs-dynamic-path-plugin");
    obs_register_module_properties(dynamic_path_properties);
    return true;
}
"""

# CMake file
cmake_txt = """cmake_minimum_required(VERSION 3.16)
project(obs-dynamic-path-plugin)

set(CMAKE_CXX_STANDARD 17)

find_package(obs REQUIRED)

add_library(${PROJECT_NAME} MODULE
    src/obs-dynamic-recording-path-plugin.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    libobs
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${OBS_INCLUDE_DIR}
)
"""

# Write files
with open(os.path.join(src_dir, "obs-dynamic-recording-path-plugin.cpp"), "w") as f:
    f.write(plugin_cpp)

with open(os.path.join(base_dir, "CMakeLists.txt"), "w") as f:
    f.write(cmake_txt)

# Create zip
zip_path = "/mnt/data/obs-dynamic-path-plugin.zip"
with zipfile.ZipFile(zip_path, "w") as zipf:
    for foldername, _, filenames in os.walk(base_dir):
        for filename in filenames:
            filepath = os.path.join(foldername, filename)
            zipf.write(filepath, os.path.relpath(filepath, base_dir))

zip_path
