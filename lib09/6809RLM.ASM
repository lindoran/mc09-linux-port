$FS:
*#map1 Set address of un-initialized variables
*
* This is the base lib09/ copy used for the default (RAM) target only.
* For ROM targets, mktarget generates this file with an explicit ORG
* derived from RAM_ORG in the target config file.
*
* For the default RAM target (CODE_ORG=$2000, no ROM/vector table),
* code and data share the same address space so no ORG is needed here.
* Globals follow the last piece of linked code in the image.
*
* If you are building a ROM target manually (without mktarget), add:
*   ORG  $xxxx    * set to RAM start for your hardware
*50000 8/4/2026
