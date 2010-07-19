TEMPLATE = lib
QT += network \
    xml
CONFIG += staticlib \
    debug_and_release

# Make sure the library gets built in the same location
# regardless of the platform. On win32 the library is
# automagically put in debug/release folders, so do the
# same for other platforms.
CONFIG(debug, debug|release) { 
    DESTDIR = debug
    TARGET = QXmppClient_d
} else {
    DESTDIR = release
    TARGET = QXmppClient
}

# Header files
HEADERS += QXmppUtils.h \
    QXmppArchiveIq.h \
    QXmppArchiveManager.h \
    QXmppBind.h \
    QXmppByteStreamIq.h \
    QXmppCallManager.h \
    QXmppClient.h \
    QXmppCodec.h \
    QXmppConfiguration.h \
    QXmppConstants.h \
    QXmppDataForm.h \
    QXmppDiscoveryIq.h \
    QXmppElement.h \
    QXmppIbbIq.h \
    QXmppInvokable.h \
    QXmppIq.h \
    QXmppJingleIq.h \
    QXmppLogger.h \
    QXmppMessage.h \
    QXmppMucIq.h \
    QXmppMucManager.h \
    QXmppNonSASLAuth.h \
    QXmppPacket.h \
    QXmppPingIq.h \
    QXmppPresence.h \
    QXmppRosterIq.h \
    QXmppRosterManager.h \
    QXmppSession.h \
    QXmppSocks.h \
    QXmppStanza.h \
    QXmppStream.h \
    QXmppStreamInitiationIq.h \
    QXmppStun.h \
    QXmppTransferManager.h \
    QXmppReconnectionManager.h \
    QXmppRemoteMethod.h \
    QXmppRpcIq.h \
    QXmppVCardManager.h \
    QXmppVCard.h \
    QXmppVersionIq.h \
    xmlrpc.h

# Source files
SOURCES += QXmppUtils.cpp \
    QXmppArchiveIq.cpp \
    QXmppArchiveManager.cpp \
    QXmppBind.cpp \
    QXmppByteStreamIq.cpp \
    QXmppCallManager.cpp \
    QXmppClient.cpp \
    QXmppCodec.cpp \
    QXmppConfiguration.cpp \
    QXmppConstants.cpp \
    QXmppDataForm.cpp \
    QXmppDiscoveryIq.cpp \
    QXmppElement.cpp \
    QXmppIbbIq.cpp \
    QXmppInvokable.cpp \
    QXmppIq.cpp \
    QXmppJingleIq.cpp \
    QXmppLogger.cpp \
    QXmppMessage.cpp \
    QXmppMucIq.cpp \
    QXmppMucManager.cpp \
    QXmppNonSASLAuth.cpp \
    QXmppPacket.cpp \
    QXmppPingIq.cpp \
    QXmppPresence.cpp \
    QXmppRosterIq.cpp \
    QXmppRosterManager.cpp \
    QXmppSession.cpp \
    QXmppSocks.cpp \
    QXmppStanza.cpp \
    QXmppStream.cpp \
    QXmppStreamInitiationIq.cpp \
    QXmppStun.cpp \
    QXmppTransferManager.cpp \
    QXmppReconnectionManager.cpp \
    QXmppRemoteMethod.cpp \
    QXmppRpcIq.cpp \
    QXmppVCardManager.cpp \
    QXmppVCard.cpp \
    QXmppVersionIq.cpp \
    xmlrpc.cpp
