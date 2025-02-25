# ethpoke

This is a simple command-line program that uses mmap to set up some GPIO registers
that control the ethernet interface pins on the CV1800b SoC of the Milk-V Duo
(64MB version) to disable ethernet and enable the I2S port. The register settings
applied by this program are described in the CV180xb-Pinout-v1.xlsx spreadsheet
on page 6 "How to switch MIPI Audio ETH into GPIO"

Run this program prior to attempting to access I2S functionality on the Duo. 
