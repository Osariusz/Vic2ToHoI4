/*Copyright (c) 2019 The Paradox Game Converters Project

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.*/



#include "Ship.h"



HoI4::Ship::Ship(const std::string& _name, const std::string& _type, const std::string& _equipment, const std::string& _owner):
	name(_name),
	type(_type),
	equipment(_equipment),
	owner(_owner)
{}


std::ostream& HoI4::operator << (std::ostream& output, const HoI4::Ship& instance)
{
	output << "\t\t\tship = { name = \"" << instance.name << "\" definition = " << instance.type << " equipment = { " << instance.equipment << " = { amount = 1 owner = " << instance.owner << " } }";
	if (instance.type == "carrier")
	{
		output << "\n";
		output << "\t\t\tair_wings = {\n";
		output << "\t\t\t\tcv_fighter_equipment_0 =  { owner = \"" << instance.owner << "\" amount = 8 } # historical: 36\n";
		output << "\t\t\t\tcv_CAS_equipment_1 =  { owner = \"" << instance.owner << "\" amount = 14 } # historical: 15\n";
		output << "\t\t\t\tcv_nav_bomber_equipment_1 =  { owner = \"" << instance.owner << "\" amount = 8 } # historical: 12\n";
		output << "\t\t\t}\n";
		output << "\t\t}\n";
	}
	else
	{
		output << " }\n";
	}	

	return output;
}