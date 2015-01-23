# stm32tivc_usb_cdc

USB CDC stack patches and packet based API skeleton with tests.
Compiles with IAR EWARM IDE.

Directory structure:

/common
  Files common for all implementations - API, ring buffer, debugging and synchronization helpers

/tools
  Python files for USB device discovering and opening, API packets serialization, echo and API tests

/stm
  Projects and libraries for STM32F4 series of MCU

/stm/lib
  Original STM32 libraries

/stm/patches
  Patches to original STM32 libraries

/stm/usb_cdc_echo
  Serial stream echo test project

/stm/usb_cdc_api
  Packet based API test project

/ti
  Projects and library patches for TivaC series of MCU

/ti/patches
  Patches to original TivaWare libraries

/ti/usb_cdc_echo
  Serial stream echo test project

/ti/usb_cdc_api
  Packet based API test project

The STM32 code does not require any other libraries to compile.
The TivaC code requires TivaWare libraries to be installed for compilation.
Set the TIVA_WARE environment variable to the TivaWare libraries installation directory
while compiling projects from /ti folder.
