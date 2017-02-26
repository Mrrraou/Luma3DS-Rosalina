const char xfer_target_xml_data[] = "<?xml version=\"1.0\"?>\n\
<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n\
<target version=\"1.0\">\n\
  <feature name=\"org.gnu.gdb.arm.core\">\n\
    <reg name=\"r0\" bitsize=\"32\"/>\n\
    <reg name=\"r1\" bitsize=\"32\"/>\n\
    <reg name=\"r2\" bitsize=\"32\"/>\n\
    <reg name=\"r3\" bitsize=\"32\"/>\n\
    <reg name=\"r4\" bitsize=\"32\"/>\n\
    <reg name=\"r5\" bitsize=\"32\"/>\n\
    <reg name=\"r6\" bitsize=\"32\"/>\n\
    <reg name=\"r7\" bitsize=\"32\"/>\n\
    <reg name=\"r8\" bitsize=\"32\"/>\n\
    <reg name=\"r9\" bitsize=\"32\"/>\n\
    <reg name=\"r10\" bitsize=\"32\"/>\n\
    <reg name=\"r11\" bitsize=\"32\"/>\n\
    <reg name=\"r12\" bitsize=\"32\"/>\n\
    <reg name=\"sp\" bitsize=\"32\" type=\"data_ptr\"/>\n\
    <reg name=\"lr\" bitsize=\"32\"/>\n\
    <reg name=\"pc\" bitsize=\"32\" type=\"code_ptr\"/>\n\
    <!-- The CPSR is register 25, rather than register 16, because\n\
         the FPA registers historically were placed between the PC\n\
         and the CPSR in the \"g\" packet.  -->\n\
    <reg name=\"cpsr\" bitsize=\"32\" regnum=\"25\"/>\n\
  </feature>\n\
  <feature name=\"org.gnu.gdb.arm.vfp\">\n\
    <reg name=\"d0\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d1\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d2\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d3\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d4\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d5\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d6\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d7\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d8\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d9\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d10\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d11\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d12\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d13\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d14\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"d15\" bitsize=\"64\" type=\"float\"/>\n\
    <reg name=\"fpscr\" bitsize=\"32\" type=\"int\" group=\"float\"/>\n\
  </feature>\n\
</target>";

size_t xfer_target_xml_len = sizeof(xfer_target_xml_data)-1;