#include "Advisor.h"
#include "CommonRegexes.h"
#include "ParserHelpers.h"



HoI4::Advisor::Advisor(std::string ideology, std::istream& theStream): ideology(std::move(ideology))
{
	registerKeyword("traits", [this](std::istream& theStream) {
		traits = commonItems::getStrings(theStream);
	});
	registerKeyword("picture", [this](std::istream& theStream) {
		picture = commonItems::getString(theStream);
	});
	registerRegex(commonItems::catchallRegex, commonItems::ignoreItem);

	parseStream(theStream);
	clearRegisteredKeywords();
}