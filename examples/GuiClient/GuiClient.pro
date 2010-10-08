include(../examples.pri)

TARGET = GuiClient
TEMPLATE = app

SOURCES += main.cpp \
    messageGraphicsItem.cpp \
    chatGraphicsScene.cpp \
    chatGraphicsView.cpp \
    chatDialog.cpp \
    mainDialog.cpp \
    rosterItemModel.cpp \
    rosterItem.cpp \
    rosterItemSortFilterProxyModel.cpp \
    utils.cpp \
    customListView.cpp \
    searchLineEdit.cpp \
    statusWidget.cpp \
    customLabel.cpp \
    statusAvatarWidget.cpp \
    statusTextWidget.cpp \
    statusToolButton.cpp \
    vCardCache.cpp \
    profileDialog.cpp \
    capabilitiesCache.cpp \
    accountsCache.cpp

HEADERS += messageGraphicsItem.h \
    chatGraphicsScene.h \
    chatGraphicsView.h \
    chatDialog.h \
    mainDialog.h \
    rosterItemModel.h \
    rosterItem.h \
    rosterItemSortFilterProxyModel.h \
    utils.h \
    customListView.h \
    searchLineEdit.h \
    statusWidget.h \
    customLabel.h \
    statusAvatarWidget.h \
    statusTextWidget.h \
    statusToolButton.h \
    vCardCache.h \
    profileDialog.h \
    capabilitiesCache.h \
    accountsCache.h

FORMS += mainDialog.ui \
    chatDialog.ui \
    statusWidget.ui \
    profileDialog.ui

QT += network \
    xml

RESOURCES += resources.qrc
