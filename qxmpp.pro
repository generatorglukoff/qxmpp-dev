include(qxmpp.pri)

TEMPLATE = subdirs

SUBDIRS = src \
          tests \
          examples

CONFIG += ordered

include(doc/doc.pri)

# Source distribution
QXMPP_ARCHIVE = qxmpp-$$QXMPP_VERSION
dist.commands = \
    $(DEL_FILE) -r $$QXMPP_ARCHIVE && \
    svn export . $$QXMPP_ARCHIVE && \
#    $(COPY_DIR) doc/html $$QXMPP_ARCHIVE/doc && \
    $(DEL_FILE) -r $$QXMPP_ARCHIVE/src/server && \
    tar czf $${QXMPP_ARCHIVE}.tar.gz $$QXMPP_ARCHIVE && \
    $(DEL_FILE) -r $$QXMPP_ARCHIVE
# dist.depends = docs
QMAKE_EXTRA_TARGETS += dist

