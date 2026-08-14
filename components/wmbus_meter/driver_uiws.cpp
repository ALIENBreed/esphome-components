/*
 Copyright (C) 2026 Fredrik Öhrström (gpl-3.0-or-later)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "../wmbus_common/meters_common_implementation.h"

namespace
{
    struct Driver : public virtual MeterCommonImplementation
    {
        Driver(MeterInfo &mi, DriverInfo &di);
    };

    static bool ok = registerDriver([](DriverInfo& di)
    {
        di.setName("uiws");
        di.setDefaultFields("name,id,status,total_m3,target_m3,timestamp");
        di.setMeterType(MeterType::WaterMeter);
        di.addDetection(MANUFACTURER_ZRI, 0x99, 0x07);
        di.setConstructor([](MeterInfo& mi, DriverInfo& di){ return std::shared_ptr<Meter>(new Driver(mi, di)); });
    });

    Driver::Driver(MeterInfo &mi, DriverInfo &di) : MeterCommonImplementation(mi, di)
    {
        // Reuse shared library fields (total_m3, meter_datetime, temperatures)
        addOptionalLibraryFields("total_m3,meter_datetime,flow_temperature_c,external_temperature_c");

        // Status & error flags: BitToString map, mask 0xffff, default "OK"
        addStringFieldWithExtractorAndLookup(
            "status",
            "Status and error flags.",
            PrintProperty::STATUS | PrintProperty::INCLUDE_TPL_STATUS,
            FieldMatcher::build()
                .set(MeasurementType::Instantaneous)
                .set(VIFRange::ErrorFlags),
            Translate::Lookup(
            {
                {
                    {
                        "ERROR_FLAGS",
                        Translate::MapType::BitToString,
                        AlwaysTrigger, MaskBits(0xffff),
                        "OK",
                        {
                            /* Add bit->string entries here if available */
                        }
                    },
                },
            }));

        // The compact-profile target/history entries are handled by the shared
        // meters implementation via storage_nr matching; no additional code needed here.
    }
}

// Tests / examples are inherited from the upstream driver definitions.
