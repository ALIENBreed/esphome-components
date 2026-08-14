/*
 * Zenner IUWS ultrasonic water meter (UIWS) driver.
 *
 * Ported from the wmbusmeters uiws.xmq driver.
 */
#include "meters_common_implementation.h"

namespace {
struct Driver : public virtual MeterCommonImplementation {
  Driver(MeterInfo &mi, DriverInfo &di);
};

static bool ok = registerDriver([](DriverInfo &di) {
  di.setName("uiws");
  di.setDefaultFields("name,id,status,total_m3,target_m3,timestamp");
  di.setMeterType(MeterType::WaterMeter);
  di.addLinkMode(LinkMode::C1);

  // uiws.xmq: mvt = ZRI,99,07
  di.addDetection(MANUFACTURER_ZRI, 0x07, 0x99);

  di.setConstructor([](MeterInfo &mi, DriverInfo &di) {
    return std::shared_ptr<Meter>(new Driver(mi, di));
  });
});

Driver::Driver(MeterInfo &mi, DriverInfo &di)
    : MeterCommonImplementation(mi, di) {
  addOptionalLibraryFields("total_m3,meter_datetime,flow_temperature_c,external_temperature_c");

  addStringFieldWithExtractorAndLookup(
      "status",
      "Status and error flags.",
      DEFAULT_PRINT_PROPERTIES | PrintProperty::STATUS_FIELD | PrintProperty::INCLUDE_TPL_STATUS,
      FieldMatcher::build().set(DifVifKey("02FD17")),
      Translate::Lookup().add(
          Translate::Rule("ERROR_FLAGS", Translate::MapType::BitToString)
              .set(MaskBits(0xffff))
              .set(DefaultMessage("OK"))));

  addNumericFieldWithExtractor(
      "unknown",
      "Unknown volume value reported by the UIWS meter.",
      DEFAULT_PRINT_PROPERTIES,
      Quantity::Volume,
      VifScaling::Auto,
      DifSignedness::Signed,
      FieldMatcher::build()
          .set(MeasurementType::Instantaneous)
          .set(VIFRange::AnyVolumeVIF)
          .set(StorageNr(1)));

  addNumericFieldWithExtractor(
      "target_date",
      "Base date of the UIWS compact profile.",
      DEFAULT_PRINT_PROPERTIES,
      Quantity::PointInTime,
      VifScaling::Auto,
      DifSignedness::Signed,
      FieldMatcher::build()
          .set(MeasurementType::Instantaneous)
          .set(VIFRange::Date)
          .set(StorageNr(30)),
      Unit::DateLT);

  addNumericFieldWithExtractor(
      "target_m3",
      "Base volume of the UIWS compact profile.",
      DEFAULT_PRINT_PROPERTIES,
      Quantity::Volume,
      VifScaling::Auto,
      DifSignedness::Signed,
      FieldMatcher::build()
          .set(MeasurementType::Instantaneous)
          .set(VIFRange::AnyVolumeVIF)
          .set(StorageNr(30)));
}
}  // namespace

KEEP_DRIVER(uiws);