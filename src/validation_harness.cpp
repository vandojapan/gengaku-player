/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

#include "validation_harness.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <set>
#include <sstream>
#include <utility>

#include "baseui.h"
#include "bitmap.h"
#include "filefinder.h"
#include "game_map.h"
#include "game_pictures.h"
#include "game_strings.h"
#include "game_variables.h"
#include "main_data.h"
#include "output.h"
#include "utils.h"
#include <lcf/rpg/eventcommand.h>

namespace {
struct Artifact {
	std::string kind;
	std::string path;
	bool success = false;
};

int sequence = 0;
std::vector<Artifact> artifacts;

bool EnvEnabled(const char* name) {
	const char* value = std::getenv(name);
	return value && value[0] != '\0' && value[0] != '0';
}

bool IsEnabled() {
	return EnvEnabled("EASYRPG_VALIDATION_LOG");
}

bool CaptureScreenshots() {
	return EnvEnabled("EASYRPG_VALIDATION_SCREENSHOTS");
}

bool IsWatchedCommand(int command_id) {
	return command_id == 3007
		|| command_id == 3008
		|| command_id == 3020
		|| command_id == 3026;
}

std::string JsonEscape(std::string_view text) {
	std::ostringstream os;
	for (unsigned char ch : text) {
		switch (ch) {
			case '"': os << "\\\""; break;
			case '\\': os << "\\\\"; break;
			case '\b': os << "\\b"; break;
			case '\f': os << "\\f"; break;
			case '\n': os << "\\n"; break;
			case '\r': os << "\\r"; break;
			case '\t': os << "\\t"; break;
			default:
				if (ch < 0x20) {
					os << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
				} else {
					os << ch;
				}
				break;
		}
	}
	return os.str();
}

void EnsureLogDirectories() {
	auto fs = FileFinder::Save();
	if (!fs) {
		return;
	}
	fs.MakeDirectory("logs", false);
	fs.MakeDirectory("logs/validation", false);
	fs.MakeDirectory("logs/validation/screens", false);
}

std::string FormatScreenPath(const ValidationHarness::Snapshot& snapshot) {
	std::ostringstream name;
	name << "logs/validation/screens/validation_"
		<< std::setw(6) << std::setfill('0') << snapshot.sequence
		<< "_cmd" << snapshot.command_id
		<< "_map" << std::setw(4) << std::setfill('0') << snapshot.map_id
		<< "_event" << std::setw(4) << std::setfill('0') << snapshot.event_id
		<< "_page" << std::setw(4) << std::setfill('0') << snapshot.page_id
		<< "_idx" << std::setw(5) << std::setfill('0') << snapshot.command_index1
		<< ".png";
	return name.str();
}

std::string MaybeCaptureScreenshot(const ValidationHarness::Snapshot& snapshot) {
	if (!CaptureScreenshots() || !DisplayUi) {
		return {};
	}

	auto bitmap = DisplayUi->CaptureScreen();
	if (!bitmap) {
		return {};
	}

	const auto path = FormatScreenPath(snapshot);
	auto os = FileFinder::Save().OpenOutputStream(path, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
	if (!os) {
		Output::Warning("ValidationHarness: Failed to open screenshot path {}", path);
		return {};
	}
	if (!bitmap->WritePNG(os)) {
		Output::Warning("ValidationHarness: Failed to write screenshot {}", path);
		return {};
	}
	return path;
}

void WriteParameters(std::ostream& os, const std::vector<std::int32_t>& params) {
	os << "[";
	for (std::size_t i = 0; i < params.size(); ++i) {
		if (i > 0) {
			os << ",";
		}
		os << params[i];
	}
	os << "]";
}

void WriteVariableDiff(std::ostream& os, const std::vector<std::int32_t>& before) {
	os << "[";
	const auto& after = Main_Data::game_variables->GetData();
	const auto max_size = std::max(before.size(), after.size());
	bool first = true;
	int written = 0;
	bool truncated = false;

	for (std::size_t i = 0; i < max_size; ++i) {
		const auto before_value = i < before.size() ? before[i] : 0;
		const auto after_value = i < after.size() ? after[i] : 0;
		if (before_value == after_value) {
			continue;
		}
		if (written >= 500) {
			truncated = true;
			break;
		}
		if (!first) {
			os << ",";
		}
		first = false;
		++written;
		os << "{\"id\":" << (i + 1)
			<< ",\"before\":" << before_value
			<< ",\"after\":" << after_value << "}";
	}
	if (truncated) {
		if (!first) {
			os << ",";
		}
		os << "{\"truncated\":true}";
	}
	os << "]";
}

void WriteStringDiff(std::ostream& os, const std::unordered_map<int, std::string>& before) {
	os << "[";
	const auto& after = Main_Data::game_strings->GetData();
	std::set<int> ids;
	for (const auto& item : before) {
		ids.insert(item.first);
	}
	for (const auto& item : after) {
		ids.insert(item.first);
	}

	bool first = true;
	int written = 0;
	bool truncated = false;
	for (int id : ids) {
		auto before_it = before.find(id);
		auto after_it = after.find(id);
		const bool had_before = before_it != before.end();
		const bool has_after = after_it != after.end();
		const std::string before_value = had_before ? before_it->second : "";
		const std::string after_value = has_after ? after_it->second : "";
		if (had_before == has_after && before_value == after_value) {
			continue;
		}
		if (written >= 200) {
			truncated = true;
			break;
		}
		if (!first) {
			os << ",";
		}
		first = false;
		++written;
		os << "{\"id\":" << id
			<< ",\"before_exists\":" << (had_before ? "true" : "false")
			<< ",\"after_exists\":" << (has_after ? "true" : "false")
			<< ",\"before\":\"" << JsonEscape(before_value)
			<< "\",\"after\":\"" << JsonEscape(after_value) << "\"}";
	}
	if (truncated) {
		if (!first) {
			os << ",";
		}
		os << "{\"truncated\":true}";
	}
	os << "]";
}

void WritePictureState(std::ostream& os, const std::vector<int>& picture_ids) {
	os << "[";
	bool first = true;
	for (int id : picture_ids) {
		if (id <= 0) {
			continue;
		}
		const auto* picture = Main_Data::game_pictures->GetPicturePtr(id);
		if (!first) {
			os << ",";
		}
		first = false;
		os << "{\"id\":" << id;
		if (!picture) {
			os << ",\"present\":false}";
			continue;
		}

		const auto& data = picture->data;
		os << ",\"present\":true"
			<< ",\"exists\":" << (picture->Exists() ? "true" : "false")
			<< ",\"request_pending\":" << (picture->IsRequestPending() ? "true" : "false")
			<< ",\"window_attached\":" << (picture->IsWindowAttached() ? "true" : "false")
			<< ",\"name\":\"" << JsonEscape(data.name) << "\""
			<< ",\"current_x\":" << data.current_x
			<< ",\"current_y\":" << data.current_y
			<< ",\"finish_x\":" << data.finish_x
			<< ",\"finish_y\":" << data.finish_y
			<< ",\"current_magnify\":" << data.current_magnify
			<< ",\"finish_magnify\":" << data.finish_magnify
			<< ",\"maniac_current_magnify_height\":" << data.maniac_current_magnify_height
			<< ",\"maniac_finish_magnify_height\":" << data.maniac_finish_magnify_height
			<< ",\"current_top_trans\":" << data.current_top_trans
			<< ",\"current_bot_trans\":" << data.current_bot_trans
			<< ",\"finish_top_trans\":" << data.finish_top_trans
			<< ",\"finish_bot_trans\":" << data.finish_bot_trans
			<< ",\"current_red\":" << data.current_red
			<< ",\"current_green\":" << data.current_green
			<< ",\"current_blue\":" << data.current_blue
			<< ",\"current_sat\":" << data.current_sat
			<< ",\"finish_red\":" << data.finish_red
			<< ",\"finish_green\":" << data.finish_green
			<< ",\"finish_blue\":" << data.finish_blue
			<< ",\"finish_sat\":" << data.finish_sat
			<< ",\"effect_mode\":" << data.effect_mode
			<< ",\"current_effect_power\":" << data.current_effect_power
			<< ",\"finish_effect_power\":" << data.finish_effect_power
			<< ",\"spritesheet_cols\":" << data.spritesheet_cols
			<< ",\"spritesheet_rows\":" << data.spritesheet_rows
			<< ",\"spritesheet_frame\":" << data.spritesheet_frame
			<< ",\"spritesheet_speed\":" << data.spritesheet_speed
			<< ",\"map_layer\":" << data.map_layer
			<< ",\"battle_layer\":" << data.battle_layer
			<< ",\"use_transparent_color\":" << (data.use_transparent_color ? "true" : "false");

		if (picture->sprite) {
			os << ",\"sprite_width\":" << picture->sprite->GetWidth()
				<< ",\"sprite_height\":" << picture->sprite->GetHeight();
			const auto& bitmap = picture->sprite->GetBitmap();
			if (bitmap) {
				os << ",\"bitmap_width\":" << bitmap->GetWidth()
					<< ",\"bitmap_height\":" << bitmap->GetHeight();
			}
		}
		os << "}";
	}
	os << "]";
}

void WriteArtifacts(std::ostream& os, std::size_t start_index) {
	os << "[";
	bool first = true;
	for (std::size_t i = start_index; i < artifacts.size(); ++i) {
		if (!first) {
			os << ",";
		}
		first = false;
		os << "{\"kind\":\"" << JsonEscape(artifacts[i].kind)
			<< "\",\"path\":\"" << JsonEscape(artifacts[i].path)
			<< "\",\"success\":" << (artifacts[i].success ? "true" : "false") << "}";
	}
	os << "]";
}
} // namespace

ValidationHarness::Snapshot ValidationHarness::BeforeCommand(
	int command_index,
	int event_id,
	int event_page_id,
	const lcf::rpg::EventCommand& command
) {
	Snapshot snapshot;
	snapshot.command_id = command.code;

	if (!IsEnabled() || !IsWatchedCommand(snapshot.command_id)) {
		return snapshot;
	}

	snapshot.enabled = true;
	snapshot.sequence = ++sequence;
	snapshot.command_index0 = command_index;
	snapshot.command_index1 = command_index + 1;
	snapshot.map_id = Game_Map::GetMapId();
	snapshot.event_id = event_id;
	snapshot.page_id = event_page_id;
	snapshot.variables_before = Main_Data::game_variables->GetData();
	snapshot.strings_before = Main_Data::game_strings->GetData();
	snapshot.artifact_start = artifacts.size();
	snapshot.parameters.assign(command.parameters.begin(), command.parameters.end());
	snapshot.command_string = ToString(command.string);

	std::set<int> picture_ids = { 1, 2, 5, 9, 10, 11, 12, 201, 999, 1000 };
	if (snapshot.command_id == 3007 && command.parameters.size() > 1) {
		picture_ids.insert(command.parameters[1]);
	} else if (snapshot.command_id == 3008 && command.parameters.size() > 3) {
		picture_ids.insert(command.parameters[3]);
	} else if (snapshot.command_id == 3026 && command.parameters.size() > 2) {
		picture_ids.insert(command.parameters[2]);
	}
	snapshot.picture_ids.assign(picture_ids.begin(), picture_ids.end());

	return snapshot;
}

void ValidationHarness::AfterCommand(const Snapshot& snapshot, bool execute_result) {
	if (!snapshot.enabled) {
		return;
	}

	EnsureLogDirectories();
	const auto screenshot_path = MaybeCaptureScreenshot(snapshot);

	auto os = FileFinder::Save().OpenOutputStream(
		"logs/validation/commands.jsonl",
		std::ios_base::out | std::ios_base::app
	);
	if (!os) {
		Output::Warning("ValidationHarness: Failed to open logs/validation/commands.jsonl");
		return;
	}

	os << "{\"sequence\":" << snapshot.sequence
		<< ",\"command_id\":" << snapshot.command_id
		<< ",\"command_index0\":" << snapshot.command_index0
		<< ",\"command_index1\":" << snapshot.command_index1
		<< ",\"map_id\":" << snapshot.map_id
		<< ",\"event_id\":" << snapshot.event_id
		<< ",\"page_id\":" << snapshot.page_id
		<< ",\"execute_result\":" << (execute_result ? "true" : "false")
		<< ",\"parameters\":";
	WriteParameters(os, snapshot.parameters);
	os << ",\"command_string\":\"" << JsonEscape(snapshot.command_string) << "\""
		<< ",\"variables_touched\":";
	WriteVariableDiff(os, snapshot.variables_before);
	os << ",\"string_variables_touched\":";
	WriteStringDiff(os, snapshot.strings_before);
	os << ",\"picture_state\":";
	WritePictureState(os, snapshot.picture_ids);
	os << ",\"screenshot_path\":\"" << JsonEscape(screenshot_path) << "\""
		<< ",\"artifacts\":";
	WriteArtifacts(os, snapshot.artifact_start);
	os << "}\n";
}

void ValidationHarness::RecordArtifactPath(std::string kind, std::string path, bool success) {
	if (!IsEnabled()) {
		return;
	}

	artifacts.push_back({ std::move(kind), std::move(path), success });
}
