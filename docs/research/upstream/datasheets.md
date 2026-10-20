# Datasheets and archived upstream files

Vendor datasheets belong to their vendors, so this repository never stores them: the MIT licence of LilyGO's repository does not extend to the vendor PDFs it carries, and the ST7789V pages are marked "Sitronix Confidential ... shall not be distributed, reproduced, or disclosed in whole or in part". Each entry below records what the research read: title, version, a public URL and the SHA-256 of the copy checked on 2026-10-20 (in the LilyGO clone at the pin, or downloaded from the URL; every download matched the copy used during the research [V]).

Issue numbers (#NNN) refer to Xinyuan-LilyGO/T-Display-S3. Tags: [V] verified against the primary source, [I] inference to confirm.

Vendors replace files behind unversioned URLs, so check the hash before trusting a page or table number:

```sh
curl -L -o ds.pdf '<url>'
shasum -a 256 ds.pdf     # macOS; sha256sum ds.pdf on Linux
```

## Archived in this folder

**`T_Display_S3-schematic-b0a5ecd.pdf`**: the T-Display-S3 v1.2 schematic, LilyGO's own work under the MIT licence of [Xinyuan-LilyGO/T-Display-S3](https://github.com/Xinyuan-LilyGO/T-Display-S3) (notice in [THIRD_PARTY_NOTICES.md](../../../THIRD_PARTY_NOTICES.md)).
- Source: `schematic/T_Display_S3.pdf`, last changed in b0a5ecd (2024-04-29, "Update schematic", #236) and unchanged at the pin 1cc41eb. Git blob `318cc4f0ae28657991ca34f7980811c64c450756`.
- SHA-256 `607b63300636521fc78fbb36ed371ac10e81e3145ffa770784f45c2bf52e08c9`, 395,064 bytes, one page (Altium export, title block dated 4/29/2024). Copied byte-for-byte from the blob [V].
- Kept here because upstream replaces the schematic under the same file name. Earlier versions stay in upstream history and are not archived: 410f030 (2022-10-10, TP4054 charger; SHA-256 `4d02ad4241a92d0e17ef1d831b44c7ed5744d600120e569d236dcd610bdd184c`) and 811c949 (2023-03-13, TP4056B; SHA-256 `c6cc87b3633d8891eb02525dcb1145c54c4d0543ad5a7b0f7777a3710e29bc4b`). Get one with `git show 811c949:schematic/T_Display_S3.pdf > sch-2023.pdf` in a T-Display-S3 clone. The 2022 file sits at `shcematic/T_Display_S3.pdf` (sic) in 410f030; c19f3ad fixed the folder name on 2022-10-18 [V].

## Board parts used by the design

**ST7789V (Sitronix): display controller**
- Title: "ST7789V 240RGB x 320 dot 262K Color with Frame Memory Single-Chip TFT Controller/Driver Datasheet".
- Version: 1.4 (2014/10), 316 pages.
- URL: LilyGO's copy, [`datasheet/ST7789V_SPEC_V1.4.pdf` at 1cc41eb](https://github.com/Xinyuan-LilyGO/T-Display-S3/blob/1cc41eb1c76b2f3215130a3dd64e7138ce00b482/datasheet/ST7789V_SPEC_V1.4.pdf), added in e662544 (2023-11-10).
- SHA-256: `94c959b15fea4a8dc9381e4c4fa94e51037a39d990f16803a03c7c2294a9cdde`, 3,198,803 bytes (git blob `2c7053f568973e5de1c8f9a257d3d94aedb2499f`).
- Used for: the 8080 write cycle TWC ≥ 66 ns (§7.4.1, p.41), hence the 10 MHz pixel clock; sleep-in current (§7.3 Table 3, p.40); reset and SLPOUT timing (Table 9 and its notes, p.48); the decode of LilyGO's init table.

**AW9364 (Awinic): backlight driver**
- Title: "AW9364 4-Channel 1-wire Dimming LED Driver with Ultra Low Dropout Current Source".
- Version: V2.4 (Feb. 2024), 17 pages.
- URL: <https://doc.awinic.com/doc/202402/d1a46d65-a45d-494f-bfc9-3d6a6a93a860.pdf>
- SHA-256: `385a9bf7561f639596ba72b710364041a313f396e3061bd97406538d0f45b20f`, 686,564 bytes.
- Used for: EN timing (start-up ≥ 20 µs, pulse high ≥ 0.5 µs, pulse low 0.5-500 µs, shutdown delay 0.8-2.5 ms, so EN low for more than 2.5 ms means off), the 16-step current table and its wrap from 1 to 16, the internal 150 kΩ pull-down on EN, 20 mA per channel and 330 µA quiescent current.

**TP4065 (TOPPOWER, NanJing Top Power ASIC Corp.): Li-ion charger**
- Title: "TP4065 (3mA-600mA Linear Li-lon Battery Charger)" (sic).
- Version: REV_2.0 (PDF dated 2023-03-04), 10 pages.
- URL: <https://www.toppwr.com/uploadfile/file/20230304/6403023707f41.pdf> (linked over plain http in #230; it now redirects to https).
- SHA-256: `c051635d7740ec188a5b6cbd9c49a0d2e8d47715c5d1324853fc598cd48b61a9`, 453,158 bytes.
- Used for: RPROG = 1050/IBAT, so R13 = 2 kΩ gives about 525 mA (spec 425-575 mA); thermal foldback; C/10 termination; 4.2 V ±1%.

**ESP32-S3 Series Datasheet (Espressif)**
- Version: 2.2 (PDF dated 2026-03-09), 87 pages.
- URL: <https://documentation.espressif.com/esp32-s3_datasheet_en.pdf> (unversioned: Espressif replaces the file on every revision).
- SHA-256: `2d5a7cb7fd559d8d972bd88db32669c0196d23f22d7afaafb0f63d099b589a3f`, 1,098,115 bytes.
- Used for: pin power domains and strapping pins (Tables 2-1 and 2-6, §3, with Tables 3-1, 3-3 and 3-4 for the strapping defaults, boot mode and VDD_SPI voltage), power-up glitches (Table 2-2), ROM boot messages (§3.3), pad drive and input levels (Table 5-4), Wi-Fi and BLE currents (Tables 5-7 and 5-8), active-mode current (Table 5-9), sleep currents (Table 5-10).

**ESP32-S3 Technical Reference Manual (Espressif)**
- Version: 1.8 (PDF dated 2026-03-04), 1531 pages.
- URL: <https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf> (the older `www.espressif.com/sites/default/files/documentation/` link redirects here).
- SHA-256: `4484bf8a69035ec42a731c58c64ada6fbd1f1618c5559409f134d9ea083f444f`, 15,215,232 bytes.
- Used for: reset types (Table 7.1-1): a deep-sleep wake is a Core Reset, and strapping pins are latched only at a Chip Reset.

**AP2112 (Diodes): 3.3 V LDO as drawn on the schematic**
- Title: "AP2112 600mA CMOS LDO Regulator with Enable".
- Version: DS39724 Rev. 2-2, 18 pages.
- URL: <https://www.diodes.com/assets/Datasheets/AP2112.pdf>
- SHA-256: `ef8d376f2ec356e29172eb9e053819a0ebdcc576dba7fc9ab0505c568427920f`, 755,270 bytes.
- Used for: quiescent current, 55 µA typical and 80 µA maximum, in the deep-sleep floor.
- [I] The fitted part is unconfirmed: the schematic says "AP2112K-3.3V(RT9013)", the maintainer says RT9013 (500 mA, #291) and RT9080 on current boards (#348). No Richtek datasheet was checked; the RT9013 figure of 25 µA came from Richtek's product page.

## Optional add-ons (roadmap "Later", not part of v0.1)

**PKLCS1212E4001-R1 (Murata): piezo sounder for audible alerts**
- Title: "Product Search Data Sheet", last updated 12/12/2020 (the copy itself warns that it may be out of date).
- URL: product page <https://www.murata.com/en-global/products/productdetail?partno=PKLCS1212E4001-R1>; the copy read is a distributor mirror, <https://static.chipdip.ru/lib/101/DOC032101147.pdf>.
- SHA-256: `f56a6cbf87f56acf83bfe274dfa4dfeb4e5d4372762615d07afc6f83f60880aa`, 294,936 bytes.
- Murata lists the part as to be discontinued (last order 2027-03-31).

**DRV2605L (Texas Instruments): haptic driver**
- Title: "DRV2605L 2- to 5.2-V Haptic Driver for LRA and ERM with Effect Library and Smart-Loop Architecture".
- Version: SLOS854D (May 2014, revised March 2018), 76 pages.
- URL: <https://www.ti.com/lit/ds/symlink/drv2605l.pdf>
- SHA-256: `0597de48de2dd1d08d3d2a801ced819c1c756f152916367608b9146c927bb290`, 2,642,481 bytes.

**AO3400A (Alpha and Omega Semiconductor): MOSFET for a vibration motor**
- Title: "AO3400A 30V N-Channel MOSFET".
- Version: Rev 3.1 (July 2023), 5 pages.
- URL: <https://www.aosmd.com/res/datasheets/AO3400A.pdf>
- SHA-256: `9c60d0b6c1ddc7609a4b788a91181468d8ffe6c3e9ba425c3d8ffc5ba88c5033`, 317,848 bytes.

**RV-3032-C7 (Micro Crystal): TCXO real-time clock**
- [I] The figures the research quoted (160 nA, ±2.5 ppm) were not checked against a datasheet copy: the vendor site refused automated downloads on 2026-10-20. Check them before buying.

## In the T-Display-S3 repository but not used

`datasheet/CST328数据手册V2.2.pdf` and `datasheet/CST816S寄存器说明-20190508.pdf` describe touch controllers; Gogadget's board has no touch panel. They are watched only because the whole `datasheet/` folder is.
