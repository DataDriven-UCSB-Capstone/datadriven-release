# UDP-Client

The UDP packet uses the following format:
| VIN | IMEI | Date | Time | Data |
|---|---|---|---|---|

These 5 parts are separated by the space character.

`sample_VIN sample_IMEI 2023-05-20 12:00:00 {INSERT BINARY DATA HERE}`


The data portion can be further broken down into:
| GNSS Data | IMU Data | OBD Data |
|---|---|---|

The GNSS data portion contains:
| Latitude | Longitude | Altitude | Speed | Vertical Speed | Heading |
|---|---|---|---|---|---|

The IMU data portion contains:
| Acceleration X | Acceleration Y | Acceleration Z | Gyroscope X | Gyroscope Y | Gyroscope Z |
|---|---|---|---|---|---|

## Example OBD Data

`06 41 A6 00 00 EB 28`

To break it down

| Byte(s) | Explanation |
|---|---|
| `06` | 6 bytes of information for this packet (not including this byte) |
| `41` | Mode 01 - Show current data `41 - 40 = 01` |
| `A6` | PID A6 - Odometer |
| `00 00 EB 28` | 6020 km = 3740.65 mi |

## Example GNSS String [Outdated/No longer used]
`$GPRMC,225300.11,A,3424.86316,N,11950.45998,W,0.04,0.00,020523,,,A,V*3C`

You can use this [GNSS Decoder](https://rl.se/gprmc) to extract data.

The data format is as follows:

| Part | Name | Example | Example Decoded |
|---|---|---|---|
| 0 | NMEA Sentence | `$GPRMC` | Recommended minimum specific GPS/Transit data |
| 1 | Time of fix | `225300.11` | 22:53:00.11 UTC = 03:53:00.11 PM Pacific Time |
| 2 | Status | `A` | Valid fix |
| 3 | Latitude | `3424.86316` | 34 degrees 24.86316 minutes |
| 4 | Latitude Direction | `N` | North |
| 5 | Longitude | `$GPRMC` | `11950.45998` | 119 degrees 50.45998 minutes |
| 6 | Longitude Direction | `W` | West |
| 7 | Speed over ground | `0.04` | 0.04 Knots = 0.046 mph |
| 8 | Course Made Good | `0.00` | 0.00 degrees true |
| 9 | Date of fix | `020523` | 02-05-2023 = Feb 5, 2023|
| 10 | Magnetic variation |  | None |
| 11 | Magnetic variation direction |  | None |
| 12 | Mode indicator | `A` | Autonomous |
| 13 | Nav status | `V` | Valid |
| 14 | Checksum | `*3C` | Valid, XOR all bytes between `$` to `*` except the `,` |

Source: [gpsd - NMEA Revealed by Eric S. Raymond](https://gpsd.gitlab.io/gpsd/NMEA.html#_rmc_recommended_minimum_navigation_information)

