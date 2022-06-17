# Scum Test Code

Confluence for SCuM Documentation and Guides: https://crystalfree.atlassian.net/wiki/spaces/SCUM/overview?homepageId=229432

## Questions?
Message Austin: austinpatel at berkeley dot edu

## Preface
- Native Windows is the only OS you can use for full SCuM development, and even a Windows VM running on macOS has been shown not to work (bootloading SCuM with nRF doesn't work through the VM for some reason). So you will need to stick to a Windows laptop or desktop PC running Windows.
- Current version of SCuM is `scm_v3c`

## Contributing
- Create a fork of this repo onto your own Github account
- Create a branch off of the `develop` branch on your fork
- Submit pull requests from your branch on your fork into the `develop` branch of this main repository once your code is working

## Build

* Install ARM Keil: https://www.keil.com/demo/eval/arm.htm, default settings (`MDK528A.EXE` and `MDK525.EXE` known to work. `MDK537.exe` (latest version as of writing) does not work out of the box since it uses compiler version 6, when we should be using compiler version 5. Perhaps this can be fixed or configured after installing.)
* Open `scm_v3c/applications/all_projects.uvmpw`
* In Keil project/workspace pane, right click desired project and click `Set as Active Project`
* Press build button to generate `.bin` file for active project (`scm_v3c/applications/<app_name>/Objects/<app_name>.bin`)

## Projects
### Existing projects
#### Read the README associated with each project for project specific setup.

freq_sweep_rx_simple: [README](scm_v3c/applications/freq_sweep_rx_simple/README.md)

freq_sweep_tx_simple: [README](scm_v3c/applications/freq_sweep_tx_simple/README.md)

hello_world: [README](scm_v3c/applications/hello_world/README.md)

log_imu: [README](scm_v3c/applications/log_imu/README.md)

continuously_cal: TODO

freq_sweep_lc_count: TODO

freq_sweep_rx: TODO

freq_sweep_tx: TODO

freq_sweep_tx_rx_transit: TODO

if_estimate_test: TODO

pingpong_test: TODO

quick_cal: TODO

channel_cal: [README](scm_v3c/applications/channel_cal/README.md)

### Creating a new project
- Create new directory `scm_v3c/applications/<project_name>`
- Copy `hello_world.c`, `hello_world.uvoptx`, and `hello_world.uvprojx` from `scm_v3c/applications/hello_world/` into new directory (replace `hello_world` with `<project_name>` in the file name for these three files)
- Find and replace all instances of `hello_world` with `<project_name>` in the file contents of `<project_name>.uvoptx`, and `<project_name>.uvprojx`
- Open `scm_v3c/applications/all_projects.uvmpw` amd in Keil press `Project > Manage > Multi-Project Workspace...` and add path to `<project_name>.uvprojx` file (it's okay to put absolute path since it will be saved as relative path)

## Bootload

You can program SCuM using either a nRF52840DK (recommended) or a Teensy.

### nRF52840DK Bootloader (for wired bootloading; this is the preferred approach)
Bootload & Wiring Guide: https://crystalfree.atlassian.net/wiki/spaces/SCUM/pages/1901559821/Sulu+Programming+With+nRF+Setup

### Teensy bootloader (for optical bootloading)
* install
    * https://www.python.org/downloads/ (`Python 3.7.4` known to work)
    * Arduino IDE (`arduino-1.8.9-windows.zip` known to work)
    * Teensyduino (`TeensyduinoInstall.exe` known to work)
* connect both Teensy+SCuM board to computer, creates a COM port for each
* run `python scm_v3c\bootload.py --image <path to .bin file>`

## OpenMote Setup
You will want to setup an [OpenMote B](https://www.industrialshields.com/shop/product/is-omb-001-openmote-b-721#attr=) if you want to either transmit packets to SCuM or receive packets from SCuM.

[Setup Guide](https://crystalfree.atlassian.net/wiki/spaces/SCUM/pages/2029879415/Basic+OpenMote+Setup+for+scum-test-code)
