#include "HoI4FocusTree.h"
#include "CommonRegexes.h"
#include "HoI4Country.h"
#include "HoI4Focus.h"
#include "HoI4Localisation.h"
#include "HoI4World.h"
#include "Log.h"
#include "OSCompatibilityLayer.h"
#include "ParserHelpers.h"
#include "SharedFocus.h"
#include "V2World/Countries/Country.h"
#include "V2World/Politics/Party.h"
#include <ranges>
using namespace std;



// this is used to cache focuses that can be just loaded from a file
static std::map<std::string, HoI4Focus> loadedFocuses;



HoI4FocusTree::HoI4FocusTree(const HoI4::Country& country): dstCountryTag(country.getTag())
{
}


constexpr bool hasMaxNeighborWars(const int numWarsWithNeighbors)
{
	return numWarsWithNeighbors >= 5;
}

void HoI4FocusTree::addGenericFocusTree(const set<string>& majorIdeologies)
{
	Log(LogLevel::Info) << "\t\tCreating generic focus tree";
	confirmLoadedFocuses();

	auto numCollectovistIdeologies = static_cast<int>(calculateNumCollectovistIdeologies(majorIdeologies));

	if (const auto& originalFocus = loadedFocuses.find("political_effort"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		newFocus->xPos = static_cast<int>((numCollectovistIdeologies * 1.5) + 16);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus political_effort");
	}

	if (numCollectovistIdeologies > 0)
	{
		if (const auto& originalFocus = loadedFocuses.find("collectivist_ethos"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
			std::string governments;
			for (auto majorIdeology: majorIdeologies)
			{
				if (majorIdeology == "democratic")
				{
					continue;
				}
				if (majorIdeology == *majorIdeologies.begin())
				{
					governments += "has_government = " + majorIdeology + "\n";
				}
				else
				{
					governments += "\t\t\thas_government = " + majorIdeology + "\n";
				}
			}
			newFocus->updateFocusElement(newFocus->available, "$GOVERNMENTS", governments);
			newFocus->xPos = -(numCollectovistIdeologies / 2) - 1;
			std::string idea;
			if (majorIdeologies.contains("democratic"))
			{
				idea = "collectivist_ethos_focus_democratic";
			}
			else
			{
				idea = "collectivist_ethos_focus_neutral";
			}
			newFocus->updateFocusElement(newFocus->completionReward, "$IDEA", idea);
			sharedFocuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus collectivist_ethos");
		}

		std::string ideolgicalFanaticsmPrereqs;
		int relativePosition = 1 - numCollectovistIdeologies;
		if (majorIdeologies.contains("fascism"))
		{
			addFascistGenericFocuses(relativePosition, majorIdeologies);
			ideolgicalFanaticsmPrereqs += " focus = paramilitarism";
			relativePosition += 2;
		}
		if (majorIdeologies.contains("communism"))
		{
			addCommunistGenericFocuses(relativePosition, majorIdeologies);
			ideolgicalFanaticsmPrereqs += " focus = political_commissars";
			relativePosition += 2;
		}
		if (majorIdeologies.contains("absolutist"))
		{
			addAbsolutistGenericFocuses(relativePosition, majorIdeologies);
			ideolgicalFanaticsmPrereqs += " focus = historical_claims_focus";
			relativePosition += 2;
		}
		if (majorIdeologies.contains("radical"))
		{
			addRadicalGenericFocuses(relativePosition, majorIdeologies);
			ideolgicalFanaticsmPrereqs += " focus = army_provides_focus";
		}

		if (const auto& originalFocus = loadedFocuses.find("ideological_fanaticism");
			 originalFocus != loadedFocuses.end())
		{
			auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= {" + ideolgicalFanaticsmPrereqs + " }");
			newFocus->xPos = 0;
			newFocus->yPos = 5;
			newFocus->relativePositionId = "collectivist_ethos";
			sharedFocuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus ideological_fanaticism");
		}
	}

	if (const auto& originalFocus = loadedFocuses.find("liberty_ethos"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		if (numCollectovistIdeologies == 0)
		{
			newFocus->mutuallyExclusive.clear();
		}
		if (!majorIdeologies.contains("democratic"))
		{
			newFocus->available = "= {\n";
			newFocus->available += "\t\t\thas_government = neutrality\n";
			newFocus->available += "\t\t}";

			newFocus->completionReward = "= {\n";
			newFocus->completionReward += "\tadd_ideas = liberty_ethos_focus_neutral\n";
			newFocus->completionReward += "}";
		}
		newFocus->xPos = (numCollectovistIdeologies + 1) / 2;
		std::string governments;
		for (auto majorIdeology: majorIdeologies)
		{
			if (majorIdeology == *majorIdeologies.begin())
			{
				governments += "has_government = " + majorIdeology + "\n";
			}
			else
			{
				governments += "\t\t\t\t\thas_government = " + majorIdeology + "\n";
			}
		}
		newFocus->updateFocusElement(newFocus->aiWillDo, "$GOVERNMENTS", governments);
		std::string noMajorDemocraticNeighbor;
		noMajorDemocraticNeighbor += "NOT = {\n";
		noMajorDemocraticNeighbor += "\t\t\t\tany_neighbor_country = {\n";
		noMajorDemocraticNeighbor += "\t\t\t\t\tis_major = yes\n";
		noMajorDemocraticNeighbor += "\t\t\t\t\thas_government = democratic\n";
		noMajorDemocraticNeighbor += "\t\t\t\t}\n";
		noMajorDemocraticNeighbor += "\t\t\t}\n";
		if (majorIdeologies.contains("democratic"))
		{
			newFocus->updateFocusElement(newFocus->aiWillDo, "#NO_MAJOR_DEMOCRATIC_NEIGHBOR", noMajorDemocraticNeighbor);
		}
		else
		{
			newFocus->removePlaceholder(newFocus->aiWillDo, "#NO_MAJOR_DEMOCRATIC_NEIGHBOR");
		}
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus liberty_ethos");
	}

	if (const auto& originalFocus = loadedFocuses.find("neutrality_focus"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		if (!majorIdeologies.contains("democratic"))
		{
			newFocus->mutuallyExclusive.clear();
		}
		newFocus->aiWillDo = "= {\n";
		newFocus->aiWillDo += "\t\t\tfactor = 1\n";
		newFocus->aiWillDo += "\t\t}";
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus neutrality_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("deterrence"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus deterrence");
	}

	if (majorIdeologies.contains("democratic"))
	{
		if (const auto& originalFocus = loadedFocuses.find("interventionism_focus"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
			sharedFocuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus interventionism_focus");
		}

		if (const auto& originalFocus = loadedFocuses.find("volunteer_corps"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
			sharedFocuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus volunteer_corps");
		}

		if (const auto& originalFocus = loadedFocuses.find("foreign_expeditions"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
			sharedFocuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus foreign_expeditions");
		}
	}

	if (const auto& originalFocus = loadedFocuses.find("why_we_fight"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		if (!majorIdeologies.contains("democratic"))
		{
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = deterrence }");
		}
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus why_we_fight");
	}

	if (const auto& originalFocus = loadedFocuses.find("technology_sharing"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		if (numCollectovistIdeologies == 0)
		{
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = why_we_fight }");
		}
		newFocus->xPos = numCollectovistIdeologies;
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus technology_sharing");
	}
	nextFreeColumn = static_cast<int>(numCollectovistIdeologies * 1.5) + ((numCollectovistIdeologies + 1) / 2) + 20;
}


void HoI4FocusTree::confirmLoadedFocuses()
{
	if (loadedFocuses.empty())
	{
		registerKeyword("focus_tree", [this](std::istream& theStream) {
		});
		registerRegex("focus|shared_focus", [this](const std::string& unused, std::istream& theStream) {
			HoI4Focus newFocus(theStream);
			loadedFocuses.insert(make_pair(newFocus.id, newFocus));
		});
		registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

		parseFile("Configurables/converterFocuses.txt");
		for (const auto& file: commonItems::GetAllFilesInFolder("Configurables/CustomizedFocusBranches"))
		{
			parseFile("Configurables/CustomizedFocusBranches/" + file);
		}
		clearRegisteredKeywords();

		createBranches();
	}
}


void HoI4FocusTree::loadFocuses(const std::string& branch)
{
	registerKeyword("focus_tree", [this](std::istream& theStream) {
	});
	registerRegex("focus|shared_focus", [this](const std::string& unused, std::istream& theStream) {
		HoI4Focus newFocus(theStream);
		loadedFocuses.insert(make_pair(newFocus.id, newFocus));
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

	parseFile("Configurables/CustomizedFocusBranches/" + branch + ".txt");
	clearRegisteredKeywords();

	createBranches();
}


void HoI4FocusTree::addBranch(const std::string& tag, const std::string& branch, HoI4::OnActions& onActions)
{
	loadFocuses(branch);

	if (!branches.contains(branch))
	{
		return;
	}
	const auto& branchFocuses = branches.at(branch);
	std::map<int, std::vector<std::string>> branchLevels;
	for (const auto& [focus, level]: branchFocuses)
	{
		branchLevels[level].push_back(focus);
	}

	int branchWidth;
	for (const auto& [level, levelFocuses]: branchLevels)
	{
		for (const auto& focus: levelFocuses)
		{
			if (const auto& originalFocus = loadedFocuses.find(focus); originalFocus != loadedFocuses.end())
			{
				const auto& newFocus = std::make_shared<HoI4Focus>(originalFocus->second);
				if (focus == branch)
				{
					branchWidth = newFocus->xPos;
					newFocus->xPos = nextFreeColumn + branchWidth / 2;
					onActions.addFocusEvent(tag, focus);
				}
				focuses.push_back(newFocus);
			}
			else
			{
				throw std::runtime_error("Could not load focus " + focus);
			}
		}
	}
	nextFreeColumn += branchWidth + 2;
}


void HoI4FocusTree::createBranches()
{
	int branchLevel = 0;
	for (const auto& [id, focus]: loadedFocuses)
	{
		if (!focus.prerequisites.empty())
		{
			continue;
		}
		branches[id].insert(std::make_pair(id, branchLevel));
		addChildrenToBranch(id, id, branchLevel);
	}
}


void HoI4FocusTree::addChildrenToBranch(const std::string& head, const std::string& id, int branchLevel)
{
	branchLevel++;
	for (const auto& [childId, childFocus]: loadedFocuses)
	{
		if (branches[head].contains(childId))
		{
			continue;
		}
		for (const auto& prerequisiteStr: childFocus.prerequisites)
		{
			const auto& prerequisites = extractIds(prerequisiteStr);
			if (prerequisites.contains(id))
			{
				branches[head].insert(std::make_pair(childId, branchLevel));
				addChildrenToBranch(head, childId, branchLevel);
			}
		}
	}
}


std::set<std::string> HoI4FocusTree::extractIds(const std::string& prerequisiteStr)
{
	std::set<std::string> ids;
	std::regex prereq("(focus\\s*=\\s*)([A-Za-z0-9_]+)");
	std::sregex_iterator end;
	std::sregex_iterator matchItr(prerequisiteStr.begin(), prerequisiteStr.end(), prereq);
	while (matchItr != end)
	{
		ids.insert((*matchItr)[2].str());
		++matchItr;
	}
	return ids;
}


size_t HoI4FocusTree::calculateNumCollectovistIdeologies(const set<string>& majorIdeologies)
{
	size_t numCollectovistIdeologies = 0;
	numCollectovistIdeologies += majorIdeologies.count("radical");
	numCollectovistIdeologies += majorIdeologies.count("absolutist");
	numCollectovistIdeologies += majorIdeologies.count("communism");
	numCollectovistIdeologies += majorIdeologies.count("fascism");
	return numCollectovistIdeologies;
}


std::string HoI4FocusTree::getMutualExclusions(const std::string& ideology, const std::set<string>& majorIdeologies)
{
	std::map<std::string, std::string> ideologyFocuses = {{"fascism", "nationalism_focus"},
		 {"communism", "internationalism_focus"},
		 {"absolutist", "absolutism_focus"},
		 {"radical", "radical_focus"}};


	std::string returnString = "= {";
	for (const auto& majorIdeology: majorIdeologies)
	{
		if (majorIdeology == ideology)
		{
			continue;
		}
		if (!ideologyFocuses.contains(majorIdeology))
		{
			continue;
		}
		returnString += " focus = " + ideologyFocuses[majorIdeology];
	}
	returnString += " }";
	return returnString;
}


void HoI4FocusTree::addFascistGenericFocuses(int relativePosition, const std::set<std::string>& majorIdeologies)
{
	if (const auto& originalFocus = loadedFocuses.find("nationalism_focus"); originalFocus != loadedFocuses.end())
	{
		const auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		newFocus->mutuallyExclusive = getMutualExclusions("fascism", majorIdeologies);
		newFocus->xPos = relativePosition;
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus nationalism_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("militarism"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus militarism");
	}

	if (const auto& originalFocus = loadedFocuses.find("military_youth"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		newFocus->completionReward = "= {\n";
		newFocus->completionReward += "\t\t\tadd_ideas = military_youth_focus\n";
		for (auto ideology: majorIdeologies)
		{
			newFocus->completionReward += "\t\t\tif = {\n";
			newFocus->completionReward += "\t\t\t\tlimit = { has_government = " + ideology + " }\n";
			newFocus->completionReward += "\t\t\t\tadd_popularity = {\n";
			newFocus->completionReward += "\t\t\t\t\tideology = " + ideology + "\n";
			newFocus->completionReward += "\t\t\t\t\tpopularity = 0.2\n";
			newFocus->completionReward += "\t\t\t\t}\n";
			newFocus->completionReward += "\t\t\t}\n";
		}
		newFocus->completionReward += "\t\t}";
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus military_youth");
	}

	if (const auto& originalFocus = loadedFocuses.find("paramilitarism"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus paramilitarism");
	}
}


void HoI4FocusTree::addCommunistGenericFocuses(int relativePosition, const std::set<std::string>& majorIdeologies)
{
	if (const auto& originalFocus = loadedFocuses.find("internationalism_focus"); originalFocus != loadedFocuses.end())
	{
		const auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		newFocus->mutuallyExclusive = getMutualExclusions("communism", majorIdeologies);
		newFocus->xPos = relativePosition;
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus internationalism_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("political_correctness"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus political_correctness");
	}

	if (const auto& originalFocus = loadedFocuses.find("indoctrination_focus"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus indoctrination_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("political_commissars"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus political_commissars");
	}
}


void HoI4FocusTree::addAbsolutistGenericFocuses(int relativePosition, const std::set<std::string>& majorIdeologies)
{
	if (const auto& originalFocus = loadedFocuses.find("absolutism_focus"); originalFocus != loadedFocuses.end())
	{
		const auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		newFocus->mutuallyExclusive = getMutualExclusions("absolutist", majorIdeologies);
		newFocus->xPos = relativePosition;
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus absolutism_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("royal_dictatorship_focus"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus royal_dictatorship_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("royal_army_tradition_focus");
		 originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus royal_army_tradition_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("historical_claims_focus"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus historical_claims_focus");
	}
}


void HoI4FocusTree::addRadicalGenericFocuses(int relativePosition, const std::set<std::string>& majorIdeologies)
{
	if (const auto& originalFocus = loadedFocuses.find("radical_focus"); originalFocus != loadedFocuses.end())
	{
		const auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		newFocus->mutuallyExclusive = getMutualExclusions("radical", majorIdeologies);
		newFocus->xPos = relativePosition;
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus radical_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("private_channels_focus"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus private_channels_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("hardfought_market_focus"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus hardfought_market_focus");
	}

	if (const auto& originalFocus = loadedFocuses.find("army_provides_focus"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = make_shared<HoI4::SharedFocus>(originalFocus->second);
		sharedFocuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus army_provides_focus");
	}
}


std::unique_ptr<HoI4FocusTree> HoI4FocusTree::makeCustomizedCopy(const HoI4::Country& country) const
{
	HoI4FocusTree newFocusTree(country);

	for (auto focus: focuses)
	{
		auto newFocus = focus->makeCustomizedCopy(newFocusTree.dstCountryTag);
		newFocusTree.addFocus(newFocus);
	}
	newFocusTree.setNextFreeColumn(nextFreeColumn);
	newFocusTree.emptyFocusTree = emptyFocusTree;

	return std::make_unique<HoI4FocusTree>(std::move(newFocusTree));
}


void HoI4FocusTree::addDemocracyNationalFocuses(std::shared_ptr<HoI4::Country> Home,
	 std::vector<shared_ptr<HoI4::Country>>& CountriesToContain,
	 HoI4::Localisation& hoi4Localisations)
{
	double WTModifier = 1;
	if (Home->getGovernmentIdeology() == "democratic")
	{
		if (const auto& rulingParty = Home->getRulingParty(); rulingParty != std::nullopt)
		{
			std::string warPol = rulingParty->getWarPolicy();
			if (warPol == "jingoism")
			{
				WTModifier = 0;
			}
			if (warPol == "pro_military")
			{
				WTModifier = 0.25;
			}
			if (warPol == "anti_military")
			{
				WTModifier = 0.5;
			}
			if (warPol == "pacifism" || warPol == "pacifist")
			{
				WTModifier = 0.5;
			}
		}
	}

	if (const auto& originalFocus = loadedFocuses.find("WarProp"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
		newFocus->updateFocusElement(newFocus->available, "$WTMODIFIER", std::to_string(0.20 * WTModifier / 1000));
		newFocus->xPos = nextFreeColumn + static_cast<int>(CountriesToContain.size()) - 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus WarProp");
	}

	if (const auto& originalFocus = loadedFocuses.find("PrepInter"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
		newFocus->updateFocusElement(newFocus->available, "$WTMODIFIER", std::to_string(0.30 * WTModifier / 1000));
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus PrepInter");
	}

	if (const auto& originalFocus = loadedFocuses.find("Lim"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
		newFocus->updateFocusElement(newFocus->available, "$WTMODIFIER", std::to_string(0.50 * WTModifier / 1000));
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus Lim");
	}

	auto relativePos = 1 - static_cast<int>(CountriesToContain.size());
	for (const auto& country: CountriesToContain)
	{
		const auto& truceUntil = Home->getTruceUntil(country->getTag());
		if (const auto& originalFocus = loadedFocuses.find("WarPlan"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), country->getTag(), hoi4Localisations);
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", country->getTag());
			newFocus->xPos = relativePos;
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", country->getTag());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus WarPlan");
		}

		if (const auto& originalFocus = loadedFocuses.find("Embargo"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), country->getTag(), hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus =  WarPlan" + Home->getTag() + country->getTag() + " }");
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", country->getTag());
			newFocus->relativePositionId += country->getTag();
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", country->getTag());
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", country->getTag());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus Embargo");
		}

		if (const auto& originalFocus = loadedFocuses.find("WAR"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), country->getTag(), hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus =  Embargo" + Home->getTag() + country->getTag() + " }");
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", country->getTag());
			newFocus->relativePositionId += country->getTag();
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", country->getTag());
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", country->getTag());
			focuses.push_back(newFocus);

			relativePos += 2;
		}
		else
		{
			throw std::runtime_error("Could not load focus WAR");
		}
	}
	nextFreeColumn += 2 * static_cast<int>(CountriesToContain.size());
}


void HoI4FocusTree::addAbsolutistEmpireNationalFocuses(std::shared_ptr<HoI4::Country> Home,
	 const std::vector<shared_ptr<HoI4::Country>>& targetColonies,
	 const std::vector<shared_ptr<HoI4::Country>>& annexationTargets,
	 HoI4::Localisation& hoi4Localisations)
{
	const auto& homeTag = Home->getTag();

	if (const auto& originalFocus = loadedFocuses.find("EmpireGlory"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->xPos = nextFreeColumn + 5;
		newFocus->yPos = 0;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus EmpireGlory");
	}

	if (const auto& originalFocus = loadedFocuses.find("StrengthenColonies"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "EmpireGlory" + homeTag;
		newFocus->xPos = -1;
		newFocus->yPos = 1;
		if (targetColonies.empty() && !annexationTargets.empty())
		{
			newFocus->aiWillDo = "= { factor = 0 }";
		}
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus StrengthenColonies");
	}

	if (const auto& originalFocus = loadedFocuses.find("StrengthenHome"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "EmpireGlory" + homeTag;
		newFocus->xPos = 1;
		newFocus->yPos = 1;
		if (annexationTargets.empty() && !targetColonies.empty())
		{
			newFocus->aiWillDo = "= { factor = 0 }";
		}
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus StrengthenHome");
	}

	if (const auto& originalFocus = loadedFocuses.find("ColonialInd"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "StrengthenColonies" + homeTag;
		newFocus->xPos = -2;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus ColonialInd");
	}

	if (const auto& originalFocus = loadedFocuses.find("ColonialHwy"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "ColonialInd" + homeTag;
		newFocus->xPos = -2;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus ColonialHwy");
	}

	if (const auto& originalFocus = loadedFocuses.find("ResourceFac"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "ColonialInd" + homeTag;
		newFocus->xPos = 0;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus ResourceFac");
	}

	if (const auto& originalFocus = loadedFocuses.find("ColonialArmy"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "StrengthenColonies" + homeTag;
		newFocus->xPos = 0;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus ColonialArmy");
	}

	// establish protectorate
	if (targetColonies.size() >= 1)
	{
		const auto& target = targetColonies.front();

		if (const auto& originalFocus = loadedFocuses.find("Protectorate"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(homeTag, target->getTag(), hoi4Localisations);
			newFocus->id = "Protectorate" + homeTag + target->getTag();
			if (const auto& truceUntil = Home->getTruceUntil(target->getTag()); truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target->getTag());
			newFocus->prerequisites.push_back("= { focus = ColonialArmy" + homeTag + " }");
			newFocus->relativePositionId = "ColonialArmy" + homeTag;
			newFocus->xPos = 0;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target->getTag());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus Protectorate");
		}
	}
	if (targetColonies.size() >= 2)
	{
		const auto& target = targetColonies.back();

		if (const auto& originalFocus = loadedFocuses.find("Protectorate"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(homeTag, target->getTag(), hoi4Localisations);
			newFocus->id = "Protectorate" + homeTag + target->getTag();
			if (const auto& truceUntil = Home->getTruceUntil(target->getTag()); truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target->getTag());
			newFocus->prerequisites.push_back(
				 "= { focus = Protectorate" + homeTag + targetColonies.front()->getTag() + " }");
			newFocus->relativePositionId = "Protectorate" + homeTag + targetColonies.front()->getTag();
			newFocus->xPos = 0;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "factor = 10", "factor = 5");
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target->getTag());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus Protectorate");
		}
	}

	if (const auto& originalFocus = loadedFocuses.find("TradeEmpire"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "ColonialInd" + homeTag;
		newFocus->xPos = -1;
		newFocus->yPos = 2;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus TradeEmpire");
	}

	if (const auto& originalFocus = loadedFocuses.find("IndHome"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "StrengthenHome" + homeTag;
		newFocus->xPos = 1;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus IndHome");
	}

	if (const auto& originalFocus = loadedFocuses.find("NationalHwy"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "IndHome" + homeTag;
		newFocus->xPos = -1;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus NationalHwy");
	}

	if (const auto& originalFocus = loadedFocuses.find("NatCollege"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "IndHome" + homeTag;
		newFocus->xPos = 1;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus NatCollege");
	}

	if (const auto& originalFocus = loadedFocuses.find("MilitaryBuildup"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "IndHome" + homeTag;
		newFocus->xPos = 2;
		newFocus->yPos = 2;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus MilitaryBuildup");
	}

	if (const auto& originalFocus = loadedFocuses.find("PrepTheBorder"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "StrengthenHome" + homeTag;
		newFocus->xPos = 4;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus PrepTheBorder");
	}

	if (const auto& originalFocus = loadedFocuses.find("NatSpirit"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(homeTag);
		newFocus->relativePositionId = "PrepTheBorder" + homeTag;
		newFocus->xPos = 0;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus NatSpirit");
	}

	// ANNEX
	if (annexationTargets.size() >= 1)
	{
		const auto& target = annexationTargets.front();

		if (const auto& originalFocus = loadedFocuses.find("Annex"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(homeTag, target->getTag(), hoi4Localisations);
			newFocus->id = "Annex" + homeTag + target->getTag();
			if (const auto& truceUntil = Home->getTruceUntil(target->getTag()); truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target->getTag());
			newFocus->prerequisites.push_back("= { focus = PrepTheBorder" + homeTag + " }");
			newFocus->relativePositionId = "PrepTheBorder" + homeTag;
			newFocus->xPos = 2;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target->getTag());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus Annex");
		}
	}
	if (annexationTargets.size() >= 2)
	{
		const auto& target = annexationTargets.back();

		if (const auto& originalFocus = loadedFocuses.find("Annex"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(homeTag, target->getTag(), hoi4Localisations);
			newFocus->id = "Annex" + homeTag + target->getTag();
			if (const auto& truceUntil = Home->getTruceUntil(target->getTag()); truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target->getTag());
			newFocus->prerequisites.push_back("= { focus = NatSpirit" + Home->getTag() + " }");
			newFocus->relativePositionId = "NatSpirit" + homeTag;
			newFocus->xPos = 1;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target->getTag());
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target->getTag());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus Annex");
		}
	}
	nextFreeColumn += 2;
}

void HoI4FocusTree::addCommunistCoupBranch(std::shared_ptr<HoI4::Country> Home,
	 const std::vector<std::shared_ptr<HoI4::Country>>& coupTargets,
	 const std::set<std::string>& majorIdeologies,
	 HoI4::Localisation& hoi4Localisations)
{
	if (!coupTargets.empty())
	{
		int coupTargetsNum = std::min(static_cast<int>(coupTargets.size()), 2);
		if (const auto& originalFocus = loadedFocuses.find("Home_of_Revolution"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
			newFocus->xPos = nextFreeColumn + coupTargetsNum - 1;
			newFocus->yPos = 0;
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus Home_of_Revolution");
		}

		for (unsigned int i = 0; i < 2; i++)
		{
			if (i < coupTargets.size())
			{
				if (const auto& originalFocus = loadedFocuses.find("Influence_"); originalFocus != loadedFocuses.end())
				{
					auto newFocus =
						 originalFocus->second.makeTargetedCopy(Home->getTag(), coupTargets[i]->getTag(), hoi4Localisations);
					newFocus->id = "Influence_" + coupTargets[i]->getTag() + "_" + Home->getTag();
					newFocus->xPos = nextFreeColumn + i * 2;
					newFocus->yPos = 1;
					newFocus->completionReward += "= {\n";
					newFocus->completionReward += "\t\t\t" + coupTargets[i]->getTag() + " = {\n";
					std::map<std::string, std::string> ideologyIdeas = {{"fascism", "fascist_influence"},
						 {"communism", "communist_influence"},
						 {"democratic", "democratic_influence"},
						 {"absolutist", "absolutist_influence"},
						 {"radical", "radical_influence"}};
					for (const auto& majorIdeology: majorIdeologies)
					{
						if (!ideologyIdeas.contains(majorIdeology))
						{
							continue;
						}
						newFocus->completionReward += "\t\t\t\tif = {\n";
						newFocus->completionReward += "\t\t\t\t\tlimit = {\n";
						newFocus->completionReward += "\t\t\t\t\t\tROOT = {\n";
						newFocus->completionReward += "\t\t\t\t\t\t\thas_government = " + majorIdeology + "\n";
						newFocus->completionReward += "\t\t\t\t\t\t}\n";
						newFocus->completionReward += "\t\t\t\t\t}\n";
						newFocus->completionReward += "\t\t\t\t\tadd_ideas = " + ideologyIdeas[majorIdeology] + "\n";
						newFocus->completionReward += "\t\t\t\t}\n";
					}
					newFocus->completionReward += "\t\t\t\tcountry_event = { id = generic.1 }\n";
					newFocus->completionReward += "\t\t\t}\n";
					newFocus->completionReward += "\t\t}";
					focuses.push_back(newFocus);
				}
				else
				{
					throw std::runtime_error("Could not load focus Influence_");
				}

				if (const auto& originalFocus = loadedFocuses.find("Coup_"); originalFocus != loadedFocuses.end())
				{
					auto newFocus =
						 originalFocus->second.makeTargetedCopy(Home->getTag(), coupTargets[i]->getTag(), hoi4Localisations);
					newFocus->id = "Coup_" + coupTargets[i]->getTag() + "_" + Home->getTag();
					newFocus->prerequisites.push_back(
						 "= { focus = Influence_" + coupTargets[i]->getTag() + "_" + Home->getTag() + " }");
					newFocus->relativePositionId = "Influence_" + coupTargets[i]->getTag() + "_" + Home->getTag();
					newFocus->xPos = 0;
					newFocus->yPos = 1;
					newFocus->updateFocusElement(newFocus->available, "$TARGET", coupTargets[i]->getTag());
					newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", coupTargets[i]->getTag());
					focuses.push_back(newFocus);
				}
				else
				{
					throw std::runtime_error("Could not load focus Coup_");
				}
			}
		}
		nextFreeColumn += 2 * coupTargetsNum;
	}
	return;
}

void HoI4FocusTree::addCommunistWarBranch(std::shared_ptr<HoI4::Country> Home,
	 std::vector<std::shared_ptr<HoI4::Country>> warTargets,
	 HoI4::Events& events,
	 HoI4::Localisation& hoi4Localisations)
{
	if (!warTargets.empty())
	{
		if (warTargets.size() > 3)
		{
			warTargets.resize(3);
		}
		int warTargetsNum = static_cast<int>(warTargets.size());
		if (const auto& originalFocus = loadedFocuses.find("StrengthCom"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
			newFocus->xPos = nextFreeColumn + warTargetsNum - 1;
			newFocus->yPos = 0;
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus StrengthCom");
		}

		if (const auto& originalFocus = loadedFocuses.find("Inter_Com_Pres"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
			newFocus->relativePositionId = "StrengthCom" + Home->getTag();
			newFocus->xPos = 0;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->completionReward, "$TEXT", newFocus->text);
			// FIXME
			// maybe add some claims?
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus Inter_Com_Pres");
		}

		for (const auto& warTarget: warTargets)
		{
			const auto& possibleWarTargetCountryName = warTarget->getName();
			std::string warTargetCountryName;
			if (possibleWarTargetCountryName)
			{
				warTargetCountryName = *possibleWarTargetCountryName;
			}
			else
			{
				Log(LogLevel::Warning) << "Could not determine war target country name for communist war focuses";
				warTargetCountryName.clear();
			}

			const auto& warTargetTag = warTarget->getTag();

			if (const auto& originalFocus = loadedFocuses.find("War"); originalFocus != loadedFocuses.end())
			{
				auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), warTargetTag, hoi4Localisations);
				newFocus->id = "War" + warTargetTag + Home->getTag();
				date dateAvailable = date("1938.1.1");
				if (const auto& relations = Home->getRelations(warTargetTag); relations)
				{
					dateAvailable.increaseByMonths((200 + relations->getRelations()) / 16);
				}
				if (const auto& truceUntil = Home->getTruceUntil(warTargetTag); truceUntil && *truceUntil > dateAvailable)
				{
					newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + truceUntil->toString());
				}
				else
				{
					newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + dateAvailable.toString() + "\n");
				}
				newFocus->xPos = nextFreeColumn;
				newFocus->yPos = 2;
				newFocus->updateFocusElement(newFocus->bypass, "$TARGET", warTargetTag);
				newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", warTargetTag);
				std::string warWithTargets;
				if (warTargets.size() > 1)
				{
					for (const auto& otherTarget: warTargets)
					{
						if (otherTarget->getTag() == warTargetTag)
						{
							continue;
						}
						if (warWithTargets.empty())
						{
							warWithTargets = "has_war_with = " + otherTarget->getTag() + "\n";
						}
						else
						{
							warWithTargets += "\t\t\t\thas_war_with = " + otherTarget->getTag() + "\n";
						}
					}
				}
				newFocus->updateFocusElement(newFocus->aiWillDo, "#WAR_WITH_TARGETS", warWithTargets);
				newFocus->updateFocusElement(newFocus->completionReward, "$TARGETNAME", warTargetCountryName);
				newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", warTargetTag);
				focuses.push_back(newFocus);
				nextFreeColumn += 2;
			}
			else
			{
				throw std::runtime_error("Could not load focus War");
			}
		}
	}
}

void HoI4FocusTree::addFascistAnnexationBranch(std::shared_ptr<HoI4::Country> Home,
	 const std::vector<std::shared_ptr<HoI4::Country>>& annexationTargets,
	 const size_t numSudetenTargets,
	 HoI4::Events& events,
	 HoI4::Localisation& hoi4Localisations)
{
	// The Following 'if' statement prevents converter from generating focuses if annexationTargets.size > 1
	// Keep this 'if' statement off until we figure out how to handle Fascist NF's
	if (const auto& originalFocus = loadedFocuses.find("The_third_way"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
		const auto& maxWidth = static_cast<int>(std::max(annexationTargets.size(), numSudetenTargets));
		newFocus->xPos = nextFreeColumn + maxWidth - 1;
		newFocus->yPos = 0;
		nextFreeColumn += 2 * maxWidth;
		// FIXME
		// Need to get Drift Defense to work
		// in modified generic focus? (tk)
		// newFocus->completionReward += "\t\t\tdrift_defence_factor = 0.5\n";
		newFocus->updateFocusElement(newFocus->completionReward, "$TEXT", newFocus->text);
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus The_third_way");
	}

	if (const auto& originalFocus = loadedFocuses.find("mil_march"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
		newFocus->relativePositionId = "The_third_way" + Home->getTag();
		newFocus->xPos = 0;
		newFocus->yPos = 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus mil_march");
	}

	int annexationFreeColumn = 1 - static_cast<int>(annexationTargets.size());
	for (const auto& target: annexationTargets)
	{
		const auto& possibleAnnexationTargetCountryName = target->getName();
		std::string annexationTargetCountryName;
		if (possibleAnnexationTargetCountryName)
		{
			annexationTargetCountryName = *possibleAnnexationTargetCountryName;
		}
		else
		{
			Log(LogLevel::Warning) << "Could not determine annexation target country name for fascist annexation focuses";
			annexationTargetCountryName.clear();
		}

		const auto& targetTag = target->getTag();

		if (const auto& originalFocus = loadedFocuses.find("_anschluss_"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), targetTag, hoi4Localisations);
			newFocus->id = Home->getTag() + "_anschluss_" + targetTag;
			date dateAvailable = date("1937.1.1");
			if (const auto& relations = Home->getRelations(targetTag); relations)
			{
				dateAvailable.increaseByMonths((200 + relations->getRelations()) / 16);
			}
			if (const auto& truceUntil = Home->getTruceUntil(targetTag); truceUntil && *truceUntil > dateAvailable)
			{
				newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + dateAvailable.toString() + "\n");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", targetTag);
			newFocus->xPos = annexationFreeColumn;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGETNAME", annexationTargetCountryName);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", targetTag);
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$EVENTID",
				 std::to_string(events.getCurrentNationFocusEventNum()));
			focuses.push_back(newFocus);
			annexationFreeColumn += 2;

			events.createAnnexEvent(*Home, *target);
		}
		else
		{
			throw std::runtime_error("Could not load focus _anschluss_");
		}
	}
}

void HoI4FocusTree::addFascistSudetenBranch(std::shared_ptr<HoI4::Country> Home,
	 const std::vector<std::shared_ptr<HoI4::Country>>& anschlussTargets,
	 const std::vector<std::shared_ptr<HoI4::Country>>& sudetenTargets,
	 const std::map<std::string, std::vector<int>>& demandedStates,
	 HoI4::Events& events,
	 HoI4::Localisation& hoi4Localisations)
{
	// if it can easily take these targets as they are not in an alliance, you can get annexation event

	if (const auto& originalFocus = loadedFocuses.find("expand_the_reich"); originalFocus != loadedFocuses.end())
	{
		auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
		if (!anschlussTargets.empty())
		{
			// if there are anschlusses, make this event require at least 1 anschluss, else, its the start of a tree
			std::string prereq = "= {";
			for (const auto& target: anschlussTargets)
			{
				prereq += " focus = " + Home->getTag() + "_anschluss_" + target->getTag();
			}
			newFocus->prerequisites.push_back(prereq + " }\n");
			newFocus->relativePositionId = "The_third_way" + Home->getTag();
			newFocus->xPos = 0;
			newFocus->yPos = 3;
		}
		else
		{
			newFocus->xPos = nextFreeColumn;
			nextFreeColumn += 2;
			newFocus->yPos = 0;
		}
		newFocus->updateFocusElement(newFocus->completionReward, "$TEXT", newFocus->text);
		addFocus(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus expand_the_reich");
	}

	int sudetenFreeColumn = 1 - static_cast<int>(sudetenTargets.size());
	for (const auto& target: sudetenTargets)
	{
		const auto& possibleSudetenTargetCountryName = target->getName();
		std::string sudetenTargetCountryName;
		if (possibleSudetenTargetCountryName)
		{
			sudetenTargetCountryName = *possibleSudetenTargetCountryName;
		}
		else
		{
			Log(LogLevel::Warning) << "Could not determine annexation target country name for fascist sudeten focuses";
			sudetenTargetCountryName.clear();
		}

		const auto& targetTag = target->getTag();

		if (const auto& originalFocus = loadedFocuses.find("_sudeten_"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), targetTag, hoi4Localisations);
			newFocus->id = Home->getTag() + "_sudeten_" + targetTag;
			date dateAvailable = date("1938.1.1");
			if (const auto& relations = Home->getRelations(targetTag); relations)
			{
				dateAvailable.increaseByMonths((200 + relations->getRelations()) / 16);
			}
			newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + dateAvailable.toString() + "\n");
			newFocus->relativePositionId = "expand_the_reich" + Home->getTag();
			newFocus->xPos = sudetenFreeColumn;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", targetTag);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGETNAME", sudetenTargetCountryName);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", targetTag);
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$EVENTID",
				 std::to_string(events.getCurrentNationFocusEventNum()));
			addFocus(newFocus);
			sudetenFreeColumn += 2;
		}
		else
		{
			throw std::runtime_error("Could not load focus _sudeten_");
		}

		// FINISH HIM
		if (const auto& originalFocus = loadedFocuses.find("_finish_"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), targetTag, hoi4Localisations);
			newFocus->id = Home->getTag() + "_finish_" + targetTag;
			if (const auto& truceUntil = Home->getTruceUntil(targetTag); truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#DATE");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", targetTag);
			newFocus->prerequisites.push_back("= { focus =  " + Home->getTag() + "_sudeten_" + targetTag + " }");
			newFocus->relativePositionId = Home->getTag() + "_sudeten_" + targetTag;
			newFocus->xPos = 0;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", targetTag);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGETNAME", sudetenTargetCountryName);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", targetTag);
			addFocus(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus _finish_");
		}

		// events
		if (demandedStates.contains(targetTag))
		{
			events.createSudetenEvent(Home->getTag(), targetTag, demandedStates.at(targetTag));
		}
	}
}


void HoI4FocusTree::addGPWarBranch(std::shared_ptr<HoI4::Country> Home,
	 const std::vector<std::shared_ptr<HoI4::Country>>& newAllies,
	 const std::vector<std::shared_ptr<HoI4::Country>>& GCTargets,
	 const std::string& ideology,
	 HoI4::Events& events,
	 Mappers::FactionNameMapper& factionNameMapper,
	 HoI4::Localisation& hoi4Localisations)
{
	const auto& ideologyShort = ideology.substr(0, 3);
	if (!newAllies.empty())
	{
		if (const auto& originalFocus = loadedFocuses.find("_Summit"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
			newFocus->id = ideologyShort + "_Summit" + Home->getTag();
			newFocus->text = ideology + "_Summit";
			newFocus->xPos = nextFreeColumn + static_cast<int>(newAllies.size()) - 1;
			newFocus->yPos = 0;
			newFocus->updateFocusElement(newFocus->completionReward, "$IDEOLOGY", ideology);
			focuses.push_back(newFocus);
			hoi4Localisations.copyFocusLocalisations("_Summit", newFocus->text);
			hoi4Localisations.updateLocalisationText(newFocus->text, "$TARGET", ideology);
			hoi4Localisations.updateLocalisationText(newFocus->text + "_desc", "$TARGET", ideology);
		}
		else
		{
			throw std::runtime_error("Could not load focus _Summit");
		}
	}

	int allianceFreeColumn = 1 - static_cast<int>(newAllies.size());
	for (const auto& newAlly: newAllies)
	{
		if (const auto& originalFocus = loadedFocuses.find("Alliance_"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(Home->getTag(), newAlly->getTag(), hoi4Localisations);
			newFocus->id = "Alliance_" + newAlly->getTag() + Home->getTag();
			newFocus->prerequisites.push_back("= { focus = " + ideologyShort + "_Summit" + Home->getTag() + " }");
			newFocus->relativePositionId = ideologyShort + "_Summit" + Home->getTag();
			newFocus->xPos = allianceFreeColumn;
			newFocus->yPos = 1;
			newFocus->updateFocusElement(newFocus->available, "$ALLY", newAlly->getTag());
			newFocus->updateFocusElement(newFocus->bypass, "$ALLY", newAlly->getTag());
			newFocus->updateFocusElement(newFocus->completionReward, "$ALLY", newAlly->getTag());
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$EVENTID",
				 std::to_string(events.getCurrentNationFocusEventNum()));
			focuses.push_back(newFocus);
			allianceFreeColumn += 2;

			events.createFactionEvents(*Home, factionNameMapper);
		}
		else
		{
			throw std::runtime_error("Could not load focus Alliance_");
		}
	}

	int GCfreeColumn = 1 - static_cast<int>(GCTargets.size());
	for (const auto& greatPower: GCTargets)
	{
		const auto& possibleWarTargetCountryName = greatPower->getName();
		std::string warTargetCountryName;
		if (possibleWarTargetCountryName)
		{
			warTargetCountryName = *possibleWarTargetCountryName;
		}
		else
		{
			Log(LogLevel::Warning) << "Could not determine war target country name for GP war focuses";
			warTargetCountryName.clear();
		}

		const auto& greatPowerTag = greatPower->getTag();
		// figuring out location of WG
		if (const auto& originalFocus = loadedFocuses.find("GP_War"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeCustomizedCopy(Home->getTag());
			for (const auto& ally: newAllies)
			{
				newFocus->prerequisites.push_back("= { focus = Alliance_" + ally->getTag() + Home->getTag() + " }");
			}
			newFocus->id = "GP_War" + greatPowerTag + Home->getTag();
			newFocus->text += greatPowerTag;
			date dateAvailable = date("1939.1.1");
			if (const auto& relations = Home->getRelations(greatPowerTag); relations)
			{
				dateAvailable.increaseByMonths((200 + relations->getRelations()) / 16);
			}
			if (const auto& truceUntil = Home->getTruceUntil(greatPowerTag); truceUntil && *truceUntil > dateAvailable)
			{
				newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + dateAvailable.toString());
			}
			if (!newAllies.empty())
			{
				newFocus->relativePositionId = ideologyShort + "_Summit" + Home->getTag();
				newFocus->xPos = GCfreeColumn;
				newFocus->yPos = 2;
			}
			else
			{
				newFocus->xPos = nextFreeColumn;
				newFocus->yPos = 0;
			}
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", greatPowerTag);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$FACTOR", std::to_string(10 - GCTargets.size() * 5));
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", greatPowerTag);
			std::string warWithTargets;
			for (const auto& otherTarget: GCTargets)
			{
				if (otherTarget->getTag() == greatPowerTag)
				{
					continue;
				}
				if (warWithTargets.empty())
				{
					warWithTargets = "has_war_with = " + otherTarget->getTag() + "\n";
				}
				else
				{
					warWithTargets += "\t\t\t\thas_war_with = " + otherTarget->getTag() + "\n";
				}
			}
			newFocus->updateFocusElement(newFocus->aiWillDo, "#WAR_WITH_TARGETS", warWithTargets);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGETNAME", warTargetCountryName);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", greatPowerTag);
			focuses.push_back(newFocus);
			GCfreeColumn += 2;
			hoi4Localisations.copyFocusLocalisations("GPWar", newFocus->text);
			hoi4Localisations.updateLocalisationText(newFocus->text, "$TARGET", greatPowerTag);
			hoi4Localisations.updateLocalisationText(newFocus->text + "_desc", "$TARGET", greatPowerTag);
		}
		else
		{
			throw std::runtime_error("Could not load focus GP_War");
		}
	}
	nextFreeColumn += 2 * static_cast<int>(max(newAllies.size(), GCTargets.size()));
}


std::map<std::string, std::set<int>> HoI4FocusTree::determineWarTargets(std::shared_ptr<HoI4::Country> theCountry,
	 const std::set<int>& stateIds,
	 const std::map<int, HoI4::State>& states)
{
	std::map<std::string, std::set<int>> targets;

	for (const auto& stateID: stateIds)
	{
		const auto& stateItr = states.find(stateID);
		if (stateItr == states.end())
		{
			continue;
		}
		const auto& state = stateItr->second;
		if (const auto& owner = state.getOwner(); owner != theCountry->getTag())
		{
			if (theCountry->isEligibleEnemy(owner))
			{
				targets[owner].emplace(stateID);
			}
		}
	}

	return targets;
}

int HoI4FocusTree::calculateNumEnemyOwnedCores(const std::set<int>& coreStates,
	 const std::map<int, HoI4::State>& states)
{
	int sumUnownedCores = 0;

	for (const auto& stateId: coreStates)
	{
		const auto& stateItr = states.find(stateId);
		if (stateItr == states.end())
		{
			continue;
		}
		const auto& state = stateItr->second;
		int numProvinces = std::min(static_cast<int>(state.getProvinces().size()), 10);
		sumUnownedCores += numProvinces;
	}

	return sumUnownedCores;
}


std::map<std::string, std::set<int>> HoI4FocusTree::addReconquestBranch(std::shared_ptr<HoI4::Country> theCountry,
	 int& numWarsWithNeighbors,
	 const std::set<std::string>& majorIdeologies,
	 const std::map<int, HoI4::State>& states,
	 HoI4::Localisation& hoi4Localisations)
{
	const auto& coreHolders = determineWarTargets(theCountry, theCountry->getCoreStates(), states);
	if (coreHolders.empty())
	{
		return coreHolders;
	}

	int sumUnownedCores = 0;
	std::map<std::string, int> provinceCount;
	for (const auto& [tag, coreStates]: coreHolders)
	{
		const auto& numProvinces = calculateNumEnemyOwnedCores(coreStates, states);
		provinceCount[tag] = numProvinces;
		sumUnownedCores += numProvinces;
	}

	numWarsWithNeighbors = std::min(static_cast<int>(coreHolders.size()), 4);

	if (const auto& originalFocus = loadedFocuses.find("reclaim_cores"); originalFocus != loadedFocuses.end())
	{
		shared_ptr<HoI4Focus> newFocus = originalFocus->second.makeCustomizedCopy(theCountry->getTag());
		newFocus->selectEffect = "= {\n";
		for (const auto& tag: coreHolders | std::views::keys)
		{
			const auto& numProvinces = provinceCount.at(tag);
			newFocus->selectEffect +=
				 "\t\t\tset_variable = { unowned_cores_@" + tag + " = " + std::to_string(numProvinces) + " }\n";
		}
		newFocus->selectEffect +=
			 "\t\t\tset_variable = { revanchism = " + std::to_string(0.00001 * sumUnownedCores) + " }\n";
		newFocus->selectEffect +=
			 "\t\t\tset_variable = { revanchism_stab = " + std::to_string(-0.000001 * sumUnownedCores) + " }\n";
		newFocus->selectEffect += "\t\t\tadd_dynamic_modifier = { modifier = revanchism }\n";
		if (majorIdeologies.contains("fascism"))
		{
			newFocus->selectEffect += "\t\t\tadd_dynamic_modifier = { modifier = revanchism_fasc }\n";
		}
		newFocus->selectEffect += "\t\t}\n";
		newFocus->xPos = nextFreeColumn + static_cast<int>(coreHolders.size()) - 1;
		focuses.push_back(newFocus);
	}
	else
	{
		throw std::runtime_error("Could not load focus reclaim_cores");
	}

	std::string fascistGovernmentCheck;
	fascistGovernmentCheck = "modifier = {\n";
	fascistGovernmentCheck += "\t\t\t\tfactor = 5\n";
	fascistGovernmentCheck += "\t\t\t\thas_government = fascism\n";
	fascistGovernmentCheck += "\t\t\t}";

	for (const auto& [target, coreStates]: coreHolders)
	{
		const auto& numProvinces = provinceCount.at(target);
		const auto& aiChance = std::to_string(std::max(static_cast<int>(0.1 * numProvinces), 1));
		const auto& truceUntil = theCountry->getTruceUntil(target);

		if (const auto& originalFocus = loadedFocuses.find("raise_matter"); originalFocus != loadedFocuses.end())
		{
			shared_ptr<HoI4Focus> newFocus =
				 originalFocus->second.makeTargetedCopy(theCountry->getTag(), target, hoi4Localisations);
			newFocus->xPos = nextFreeColumn;
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			if (majorIdeologies.contains("fascism"))
			{
				std::string fascismPopularityCheck;
				fascismPopularityCheck = "modifier = {\n";
				fascismPopularityCheck += "\t\t\t\tfactor = 0\n";
				fascismPopularityCheck += "\t\t\t\tNOT = { has_government = fascism }\n";
				fascismPopularityCheck += "\t\t\t\tNOT = { fascism > 0.35 }\n";
				fascismPopularityCheck += "\t\t\t}";
				newFocus->updateFocusElement(newFocus->aiWillDo, "#FASCPOP", fascismPopularityCheck);
				newFocus->updateFocusElement(newFocus->aiWillDo, "#FASCGOV", fascistGovernmentCheck);
			}
			else
			{
				newFocus->completionReward = "= {\n";
				newFocus->completionReward += "\t\t\tadd_stability = 0.0001\n";
				newFocus->completionReward += "\t\t\tadd_political_power = 150\n";
				newFocus->completionReward += "\t\t\tadd_timed_idea = { idea = generic_military_industry days = 180 }\n";
				newFocus->completionReward += "\t\t}";
				newFocus->removePlaceholder(newFocus->aiWillDo, "#FASCPOP");
				newFocus->removePlaceholder(newFocus->aiWillDo, "#FASCGOV");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$POPULARITY",
				 std::to_string(0.000001 * numProvinces));
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$REVANCHISM", aiChance);
			focuses.push_back(newFocus);
			nextFreeColumn += 2;
		}
		else
		{
			throw std::runtime_error("Could not load focus raise_matter");
		}

		if (const auto& originalFocus = loadedFocuses.find("build_public_support"); originalFocus != loadedFocuses.end())
		{
			shared_ptr<HoI4Focus> newFocus =
				 originalFocus->second.makeTargetedCopy(theCountry->getTag(), target, hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = raise_matter" + theCountry->getTag() + target + " }");
			newFocus->relativePositionId += target;
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			if (majorIdeologies.contains("fascism"))
			{
				std::string fascismPopularityCheck;
				fascismPopularityCheck = "modifier = {\n";
				fascismPopularityCheck += "\t\t\t\tfactor = 0\n";
				fascismPopularityCheck += "\t\t\t\tNOT = { has_government = fascism }\n";
				fascismPopularityCheck += "\t\t\t\tNOT = { fascism > 0.4 }\n";
				fascismPopularityCheck += "\t\t\t}";
				newFocus->updateFocusElement(newFocus->aiWillDo, "#FASCPOP", fascismPopularityCheck);
				newFocus->updateFocusElement(newFocus->aiWillDo, "#FASCGOV", fascistGovernmentCheck);
			}
			else
			{
				newFocus->completionReward = " = {\n";
				newFocus->completionReward += "\t\t\tadd_stability = 0.0001\n";
				newFocus->completionReward += "\t\t\tadd_war_support = $WARSUPPORT\n";
				newFocus->completionReward += "\t\t\tadd_timed_idea = { idea = generic_rapid_mobilization days = 180 }\n";
				newFocus->completionReward += "\t\t}";
				newFocus->removePlaceholder(newFocus->aiWillDo, "#FASCPOP");
				newFocus->removePlaceholder(newFocus->aiWillDo, "#FASCGOV");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$POPULARITY",
				 std::to_string(0.000001 * numProvinces));
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$WARSUPPORT",
				 std::to_string(0.00001 * numProvinces));
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TAG", theCountry->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "$REVANCHISM", aiChance);
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus build_public_support");
		}

		if (const auto& originalFocus = loadedFocuses.find("territory_or_war"); originalFocus != loadedFocuses.end())
		{
			shared_ptr<HoI4Focus> newFocus =
				 originalFocus->second.makeTargetedCopy(theCountry->getTag(), target, hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = build_public_support" + theCountry->getTag() + target + " }");
			newFocus->relativePositionId += target;
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			if (majorIdeologies.contains("fascism"))
			{
				newFocus->updateFocusElement(newFocus->aiWillDo, "#FASCGOV", fascistGovernmentCheck);
			}
			else
			{
				newFocus->removePlaceholder(newFocus->aiWillDo, "#FASCGOV");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TAG", theCountry->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "$REVANCHISM", aiChance);
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus territory_or_war");
		}

		if (const auto& originalFocus = loadedFocuses.find("war_plan"); originalFocus != loadedFocuses.end())
		{
			shared_ptr<HoI4Focus> newFocus =
				 originalFocus->second.makeTargetedCopy(theCountry->getTag(), target, hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = territory_or_war" + theCountry->getTag() + target + " }");
			newFocus->relativePositionId += target;
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			if (majorIdeologies.contains("fascism"))
			{
				newFocus->updateFocusElement(newFocus->aiWillDo, "#FASCGOV", fascistGovernmentCheck);
			}
			else
			{
				newFocus->removePlaceholder(newFocus->aiWillDo, "#FASCGOV");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TAG", theCountry->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo, "$REVANCHISM", aiChance);
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus war_plan");
		}

		if (const auto& originalFocus = loadedFocuses.find("declare_war"); originalFocus != loadedFocuses.end())
		{
			shared_ptr<HoI4Focus> newFocus =
				 originalFocus->second.makeTargetedCopy(theCountry->getTag(), target, hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = war_plan" + theCountry->getTag() + target + " }");
			newFocus->relativePositionId += target;
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			if (majorIdeologies.contains("fascism"))
			{
				newFocus->updateFocusElement(newFocus->aiWillDo, "#FASCGOV", fascistGovernmentCheck);
			}
			else
			{
				newFocus->removePlaceholder(newFocus->aiWillDo, "#FASCGOV");
			}
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target);
			std::string coresString;
			for (const auto& stateId: coreStates)
			{
				coresString += std::to_string(stateId) + " ";
			}
			newFocus->updateFocusElement(newFocus->completionReward, "$CORE_STATES", coresString);
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TAG", theCountry->getTag());
			newFocus->updateFocusElement(newFocus->aiWillDo,
				 "$REVANCHISM",
				 std::to_string(std::max(static_cast<int>(numProvinces), 1)));
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus declare_war");
		}

		if (const auto& originalFocus = loadedFocuses.find("cleanup_revanchism"); originalFocus != loadedFocuses.end())
		{
			shared_ptr<HoI4Focus> newFocus =
				 originalFocus->second.makeTargetedCopy(theCountry->getTag(), target, hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = declare_war" + theCountry->getTag() + target + " }");
			newFocus->relativePositionId += target;
			newFocus->updateFocusElement(newFocus->available, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", target);
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$REVANCHISM",
				 std::to_string(0.000005 * numProvinces));
			newFocus->updateFocusElement(newFocus->completionReward,
				 "$STABILITY",
				 std::to_string(0.0000005 * numProvinces));
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus cleanup_revanchism");
		}
	}

	return coreHolders;
}

double HoI4FocusTree::getMaxConquerValue(const std::vector<HoI4::AIStrategy>& conquerStrategies)
{
	const auto& maxConquerValueItr = std::max_element(conquerStrategies.begin(),
		 conquerStrategies.end(),
		 [](const HoI4::AIStrategy& a, const HoI4::AIStrategy& b) {
			 return a.getValue() < b.getValue();
		 });
	return maxConquerValueItr->getValue();
}

std::set<std::string> HoI4FocusTree::addConquerBranch(std::shared_ptr<HoI4::Country> theCountry,
	 int& numWarsWithNeighbors,
	 const std::set<std::string>& majorIdeologies,
	 const std::map<std::string, std::set<int>>& coreHolders,
	 const std::map<int, HoI4::State>& states,
	 HoI4::Localisation& hoi4Localisations)
{
	// less than this % of highest strategy value means theCountry doesn't get a focus on this target
	// compensating for the Vic2 AI error of wanting to conquer provinces across ocean(s)
	constexpr double strategyValueTreshold = 0.1;
	std::string tag = theCountry->getTag();
	std::set<std::string> conquerTags;

	const auto& conquerStrategies = theCountry->getConquerStrategies();
	if (conquerStrategies.empty())
	{
		return conquerTags;
	}

	for (const auto& strategy: conquerStrategies)
	{
		if (strategy.getID() == tag)
		{
			continue;
		}

		double relativeValue = strategy.getValue() / getMaxConquerValue(conquerStrategies);
		if (hasMaxNeighborWars(numWarsWithNeighbors) || relativeValue < strategyValueTreshold)
		{
			break;
		}
		if (!theCountry->isEligibleEnemy(strategy.getID()))
		{
			continue;
		}
		if (coreHolders.contains(strategy.getID()))
		{
			continue;
		}
		const auto& claimsHolders = determineWarTargets(theCountry, theCountry->getClaimedStates(), states);
		std::optional<int> newClaim;
		if (!claimsHolders.contains(strategy.getID()))
		{
			newClaim = strategy.getClaimedState();
		}
		if (!newClaim)
		{
			continue;
		}
		const auto& stateId = std::to_string(*newClaim);

		conquerTags.insert(strategy.getID());
		numWarsWithNeighbors++;

		const auto& truceUntil = theCountry->getTruceUntil(strategy.getID());
		if (const auto& originalFocus = loadedFocuses.find("border_disputes"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(tag, strategy.getID(), hoi4Localisations);
			newFocus->relativePositionId.clear();
			newFocus->updateFocusElement(newFocus->available, "$TARGET", strategy.getID());
			if (truceUntil)
			{
				newFocus->updateFocusElement(newFocus->available, "#TRUCE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->removePlaceholder(newFocus->available, "#TRUCE");
			}
			newFocus->updateFocusElement(newFocus->available, "#OWNSCLAIM", "owns_state = " + stateId);
			newFocus->xPos = nextFreeColumn;
			newFocus->yPos = 0;
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", strategy.getID());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus border_disputes");
		}

		if (const auto& originalFocus = loadedFocuses.find("assert_claims"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(tag, strategy.getID(), hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = border_disputes" + tag + strategy.getID() + " }");
			newFocus->relativePositionId += strategy.getID();
			newFocus->updateFocusElement(newFocus->available, "$TARGET", strategy.getID());
			date dateAvailable = date("1936.1.1");
			if (const auto& relations = theCountry->getRelations(strategy.getID()); relations)
			{
				dateAvailable.increaseByMonths((200 + relations->getRelations()) / 8);
			}
			if (const auto& truceUntil = theCountry->getTruceUntil(strategy.getID());
				 truceUntil && *truceUntil > dateAvailable)
			{
				newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + truceUntil->toString());
			}
			else
			{
				newFocus->updateFocusElement(newFocus->available, "#DATE", "date > " + dateAvailable.toString());
			}
			newFocus->updateFocusElement(newFocus->available, "#OWNSCLAIM", "owns_state = " + stateId);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", strategy.getID());
			newFocus->updateFocusElement(newFocus->completionReward, "#ADDCLAIM", "add_state_claim = " + stateId);
			newFocus->updateFocusElement(newFocus->aiWillDo, "$TARGET", strategy.getID());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus assert_claims");
		}

		if (const auto& originalFocus = loadedFocuses.find("prepare_for_war"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(tag, strategy.getID(), hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = assert_claims" + tag + strategy.getID() + " }");
			newFocus->relativePositionId += strategy.getID();
			newFocus->updateFocusElement(newFocus->available, "$TARGET", strategy.getID());
			newFocus->updateFocusElement(newFocus->available, "#OWNSCLAIM", "owns_state = " + stateId);
			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", strategy.getID());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus prepare_for_war");
		}

		if (const auto& originalFocus = loadedFocuses.find("neighbor_war"); originalFocus != loadedFocuses.end())
		{
			auto newFocus = originalFocus->second.makeTargetedCopy(tag, strategy.getID(), hoi4Localisations);
			newFocus->prerequisites.clear();
			newFocus->prerequisites.push_back("= { focus = prepare_for_war" + tag + strategy.getID() + " }");
			newFocus->relativePositionId += strategy.getID();
			newFocus->updateFocusElement(newFocus->available, "$TARGET", strategy.getID());
			newFocus->updateFocusElement(newFocus->available, "#OWNSCLAIM", "owns_state = " + stateId);
			newFocus->updateFocusElement(newFocus->completionReward, "$TARGET", strategy.getID());

			std::string claimsString = "claimed_states";
			if (const auto& claimedStatesItr = claimsHolders.find(strategy.getID());
				 claimedStatesItr != claimsHolders.end())
			{
				claimsString = "{ ";
				for (const auto& stateId: claimedStatesItr->second)
				{
					claimsString += std::to_string(stateId) + " ";
				}
				claimsString += "}";
			}
			else if (newClaim)
			{
				claimsString = "{ " + std::to_string(*newClaim) + " }";
			}
			newFocus->updateFocusElement(newFocus->completionReward, "$CLAIMED_STATES", claimsString);

			newFocus->updateFocusElement(newFocus->bypass, "$TARGET", strategy.getID());
			focuses.push_back(newFocus);
		}
		else
		{
			throw std::runtime_error("Could not load focus neighbor_war");
		}

		nextFreeColumn += 2;
	}

	return conquerTags;
}

void HoI4FocusTree::addIntegratePuppetsBranch(const std::string& tag,
	 const std::map<std::string, std::string>& puppets,
	 HoI4::Localisation& hoi4Localisations)
{
	if (const auto& originalFocus = loadedFocuses.find("integrate_satellite"); originalFocus != loadedFocuses.end())
	{
		int yPos = 0;

		for (const auto& puppet: puppets | std::views::keys)
		{
			shared_ptr<HoI4Focus> newFocus = originalFocus->second.makeTargetedCopy(tag, puppet, hoi4Localisations);
			newFocus->xPos = nextFreeColumn;
			newFocus->yPos = yPos;
			newFocus->updateFocusElement(newFocus->selectEffect, "#TARGET", puppet);
			newFocus->updateFocusElement(newFocus->bypass, "#TARGET", puppet);
			newFocus->updateFocusElement(newFocus->completionReward, "#TARGET", puppet);
			focuses.push_back(newFocus);

			yPos++;
		}
	}
	else
	{
		throw std::runtime_error("Could not load focus integrate_satellite");
	}

	nextFreeColumn += 2;
}

void HoI4FocusTree::removeFocus(const string& id)
{
	focuses.erase(std::remove_if(focuses.begin(),
							focuses.end(),
							[id](std::shared_ptr<HoI4Focus> focus) {
								return focus->id == id;
							}),
		 focuses.end());
}