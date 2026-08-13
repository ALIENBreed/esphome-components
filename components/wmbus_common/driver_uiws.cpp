#include"meters_common_implementation.h"

namespace
{
    struct Driver : public virtual MeterCommonImplementation
    {
        Driver(MeterInfo &mi, DriverInfo &di);
    };

    static bool ok = registerDriver([](DriverInfo&di)
    {
        di.setName("uiws");
        di.setDefaultFields("name,id,status,total_m3,target_m3,timestamp");
        di.setMeterType(MeterType::WaterMeter);
        di.addLinkMode(LinkMode::C1);
        // uiws.xmq: mvt = ZRI,99,07. addDetection = manufacturer,type,version.
        di.addDetection(MANUFACTURER_ZRI, 0x07, 0x99);
        di.setConstructor([](MeterInfo& mi, DriverInfo& di){ return std::shared_ptr<Meter>(new Driver(mi, di)); });
    });

    Driver::Driver(MeterInfo &mi, DriverInfo &di) : MeterCommonImplementation(mi, di)
    {
        addOptionalLibraryFields("total_m3,meter_datetime");
        addOptionalLibraryFields("flow_temperature_c,external_temperature_c");

        // uiws.xmq: status = Instantaneous + ErrorFlags (02 FD 17).
        addStringFieldWithExtractorAndLookup(
            "status",
            "Status and error flags.",
            DEFAULT_PRINT_PROPERTIES | PrintProperty::STATUS |
                PrintProperty::INCLUDE_TPL_STATUS,
            FieldMatcher::build().set(DifVifKey("02FD17")),
            {
                {
                    {
                        "ERROR_FLAGS",
                        Translate::MapType::BitToString,
                        AlwaysTrigger, MaskBits(0xffff),
                        "OK",
                        {
                        }
                    },
                },
            });

        // uiws.xmq: AnyVolumeVIF, storage_nr=1 -> unknown_m3.
        addNumericFieldWithExtractor(
            "unknown",
            "Unknown volume value reported by the UIWS meter.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Volume,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
                .set(MeasurementType::Instantaneous)
                .set(VIFRange::AnyVolumeVIF)
                .set(StorageNr(1))
            );

        // uiws.xmq: Date, storage_nr=30 -> target_date.
        addNumericFieldWithExtractor(
            "target",
            "Base date of compact profile (most recent monthly value).",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::PointInTime,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
                .set(MeasurementType::Instantaneous)
                .set(VIFRange::Date)
                .set(StorageNr(30)),
            Unit::DateLT
            );

        // uiws.xmq: AnyVolumeVIF, storage_nr=30 -> target_m3.
        addNumericFieldWithExtractor(
            "target",
            "Base volume of the UIWS compact profile.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Volume,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
                .set(MeasurementType::Instantaneous)
                .set(VIFRange::AnyVolumeVIF)
                .set(StorageNr(30))
            );
    }
}

// Reference telegram supplied by the user:
//
// 6e44496a1935416199077a480060252f2f_
// 046d2a5347370413bbca00004413a3210000820f6c4137840f13e4c30000
// 8d0f93132e34fec1a1000098820000d86900004850000020390000a3210000
// 820d00000000000000000080000000800000008002fd1700002f2f2f2f2f2f2f
//
// Expected core values after decryption:
// total_m3=51.899, unknown_m3=8.611, target_m3=50.148,
// target_date=2026-07-01, meter_datetime=2026-07-07 19:42, status=OK.
