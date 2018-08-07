
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://as58xx-cl.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../as58xx-cl.conf ${D}${sysconfdir}/sensors.d/as58xx-cl.conf
}
