#include "profileDialog.h"
#include "ui_profileDialog.h"
#include "utils.h"

#include "QXmppClient.h"
#include "QXmppVersionIq.h"
#include "QXmppVersionManager.h"
#include "QXmppRosterManager.h"
#include "QXmppUtils.h"
#include "QXmppEntityTimeManager.h"
#include "QXmppEntityTimeIq.h"

profileDialog::profileDialog(QWidget *parent, const QString& bareJid, QXmppClient& client) :
    QDialog(parent, Qt::WindowTitleHint|Qt::WindowSystemMenuHint),
    ui(new Ui::profileDialog), m_bareJid(bareJid), m_xmppClient(client)
{
    ui->setupUi(this);

    bool check = connect(&m_xmppClient.versionManager(), SIGNAL(versionReceived(const QXmppVersionIq&)),
            SLOT(versionReceived(const QXmppVersionIq&)));
    Q_ASSERT(check);

    QXmppEntityTimeManager* timeManager = m_xmppClient.findExtension<QXmppEntityTimeManager>();

    if(timeManager)
    {
        check = connect(timeManager, SIGNAL(timeReceived(const QXmppEntityTimeIq&)),
            SLOT(timeReceived(const QXmppEntityTimeIq&)));
        Q_ASSERT(check);
    }

    QStringList resources = m_xmppClient.rosterManager().getResources(bareJid);
    foreach(QString resource, resources)
    {
        QString jid = bareJid + "/" + resource;
        m_xmppClient.versionManager().requestVersion(jid);
        if(timeManager)
            timeManager->requestTime(jid);
    }
    updateText();
}

profileDialog::~profileDialog()
{
    delete ui;
}

void profileDialog::setAvatar(const QImage& image)
{
    ui->label_avatar->setPixmap(QPixmap::fromImage(image));
}

void profileDialog::setBareJid(const QString& bareJid)
{
    ui->label_jid->setText(bareJid);
    setWindowTitle(bareJid);
}

void profileDialog::setFullName(const QString& fullName)
{
    if(fullName.isEmpty())
        ui->label_fullName->hide();
    else
        ui->label_fullName->show();

    ui->label_fullName->setText(fullName);
}

void profileDialog::setStatusText(const QString& status)
{
    ui->label_status->setText(status);
}

void profileDialog::versionReceived(const QXmppVersionIq& ver)
{
    m_versions[jidToResource(ver.from())] = ver;
    if(ver.type() == QXmppIq::Result)
        updateText();
}

void profileDialog::timeReceived(const QXmppEntityTimeIq& time)
{
    m_time[jidToResource(time.from())] = time;
    if(time.type() == QXmppIq::Result)
        updateText();
}

void profileDialog::updateText()
{
    QStringList resources = m_xmppClient.rosterManager().getResources(m_bareJid);
    QString statusText;
    for(int i = 0; i < resources.count(); ++i)
    {
        QString resource = resources.at(i);
        statusText += "<B>Resource: </B>" + resource;
        statusText += "</B><BR>";
        QXmppPresence presence = m_xmppClient.rosterManager().getPresence(m_bareJid, resource);
        statusText += "<B>Status: </B>" + presenceToStatusText(presence);
        statusText += "<BR>";
        if(m_versions.contains(resource))
            statusText += "<B>Software: </B>" + QString("%1 %2 %3").
                          arg(m_versions[resource].name()).
                          arg(m_versions[resource].version()).
                          arg(m_versions[resource].os());
        statusText += "<BR>";

        if(m_time.contains(resource))
            statusText += "<B>Time: </B>" + QString("utc=%1 [tzo=%2]").
                          arg(m_time[resource].utc()).
                          arg(m_time[resource].tzo());

        if(i < resources.count() - 1) // skip for the last item
            statusText += "<BR>";
    }
    setStatusText(statusText);
}

