CONFIG += debug_and_release

CONFIG(debug, debug|release) {
    QXMPP_INCLUDE_DIR = $$PWD/source
    QXMPP_LIBRARY_DIR = $$PWD/source/debug
    QXMPP_LIB = qxmpp_d
} else {
    QXMPP_INCLUDE_DIR = $$PWD/source
    QXMPP_LIBRARY_DIR = $$PWD/source/release
    QXMPP_LIB = qxmpp
}

