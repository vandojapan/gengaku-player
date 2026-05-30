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

#ifndef EP_VALIDATION_HARNESS_H
#define EP_VALIDATION_HARNESS_H

#include <cstddef>
#include <cstdint>
#include <lcf/rpg/eventcommand.h>
#include <string>
#include <unordered_map>
#include <vector>

class ValidationHarness {
public:
	struct Snapshot {
		bool enabled = false;
		int sequence = 0;
		int command_id = 0;
		int command_index0 = 0;
		int command_index1 = 0;
		int map_id = 0;
		int event_id = 0;
		int page_id = 0;
		std::vector<std::int32_t> parameters;
		std::string command_string;
		std::vector<std::int32_t> variables_before;
		std::unordered_map<int, std::string> strings_before;
		std::vector<int> picture_ids;
		std::size_t artifact_start = 0;
	};

	static Snapshot BeforeCommand(
		int command_index,
		int event_id,
		int event_page_id,
		const lcf::rpg::EventCommand& command
	);
	static void AfterCommand(const Snapshot& snapshot, bool execute_result);
	static void RecordArtifactPath(std::string kind, std::string path, bool success);
};

#endif
