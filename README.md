# Scum Test Code

Confluence for SCuM Documentation and Guides: https://crystalfree.atlassian.net/wiki/spaces/SCUM/overview?homepageId=229432

## Preface
- Windows is the recommended OS for SCuM development.
- Current version of SCuM is `scm_v3c`

## Contributing
- Create a fork of this repo onto your own Github account
- Create a branch off of the `develop` branch
- Submit pull requests from your branch into the `develop` branch

## Build

* Install ARM Keil: https://www.keil.com/demo/eval/arm.htm, default settings (`MDK528A.EXE` known to work)
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

### Creating a new project
- Create new directory `scm_v3c/applications/<project_name>`
- Copy `hello_world.c`, `hello_world.uvoptx`, and `hello_world.uvprojx` from `scm_v3c/applications/hello_world/` into new directory (replace `hello_world` with `<project_name>` in the file name for these three files)
- Find and replace all instances of `hello_world` with `<project_name>` in the file contents of `<project_name>.uvoptx`, and `<project_name>.uvprojx`
- Open `scm_v3c/applications/all_projects.uvmpw` amd in Keil press `Project > Manage > Multi-Project Workspace...` and add path to `<project_name>.uvprojx` file (it's okay to put absolute path since it will be saved as relative path)

## Bootload

### nRF52840DK Bootloader
Bootload & Wiring Guide: https://crystalfree.atlassian.net/wiki/spaces/SCUM/pages/1901559821/Sulu+Programming+With+nRF+Setup

### Teensy bootloader
* install
    * https://www.python.org/downloads/ (`Python 3.7.4` known to work)
    * Arduino IDE (`arduino-1.8.9-windows.zip` known to work)
    * Teensyduino (`TeensyduinoInstall.exe` known to work)
* connect both Teensy+SCuM board to computer, creates a COM port for each
* run `python scm_v3c\bootload.py --image <path to .bin file>`

## OpenMote Setup
[Guide](https://crystalfree.atlassian.net/wiki/spaces/SCUM/pages/2029879415/Basic+OpenMote+Setup+for+scum-test-code)