
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://questone2.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../questone2.conf ${D}${sysconfdir}/sensors.d/questone2.conf
}
