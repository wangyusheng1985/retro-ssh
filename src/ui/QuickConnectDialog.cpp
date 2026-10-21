#include "QuickConnectDialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QInputDialog>
#include <QDir>
#include <QFile>

QuickConnectDialog::QuickConnectDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Connect to Remote Host");
    setFixedSize(400, 175);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QHBoxLayout *rootHl = new QHBoxLayout(this);
    rootHl->setContentsMargins(14, 14, 14, 14);
    rootHl->setSpacing(12);

    // Left Icon (Dual PC)
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icons/dual_pc.svg").pixmap(44, 38));
    iconLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    rootHl->addWidget(iconLabel);

    // Center Form Grid
    QGridLayout *grid = new QGridLayout();
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    grid->addWidget(new QLabel("Host Name:", this), 0, 0);
    m_hostEdit = new QLineEdit("10.9.34.84", this);
    grid->addWidget(m_hostEdit, 0, 1);

    grid->addWidget(new QLabel("User Name:", this), 1, 0);
    m_userEdit = new QLineEdit("root", this);
    grid->addWidget(m_userEdit, 1, 1);

    grid->addWidget(new QLabel("Port Number:", this), 2, 0);
    m_portEdit = new QLineEdit("22", this);
    m_portEdit->setFixedWidth(160);
    grid->addWidget(m_portEdit, 2, 1, Qt::AlignLeft);

    grid->addWidget(new QLabel("Authentication Method:", this), 3, 0);
    m_authCombo = new QComboBox(this);
    m_authCombo->addItems({
        "Password",
        "Public Key",
        "SecurID",
        "PAM",
        "Keyboard Interactive",
        "<Profile Settings>"
    });
    m_authCombo->setCurrentText("<Profile Settings>");
    grid->addWidget(m_authCombo, 3, 1);

    rootHl->addLayout(grid, 1);

    // Right Buttons (Connect & Cancel stacked vertically)
    QVBoxLayout *btnVl = new QVBoxLayout();
    btnVl->setSpacing(6);

    m_connectBtn = new QPushButton("Connect", this);
    m_connectBtn->setDefault(true);
    m_connectBtn->setFixedWidth(75);
    btnVl->addWidget(m_connectBtn);

    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setFixedWidth(75);
    btnVl->addWidget(m_cancelBtn);

    btnVl->addStretch();
    rootHl->addLayout(btnVl);

    connect(m_hostEdit, &QLineEdit::textChanged, this, &QuickConnectDialog::onHostTextChanged);

    connect(m_connectBtn, &QPushButton::clicked, this, [this]() {
        QString method = m_authCombo->currentText();

        // 1. If user explicitly chooses "Public Key": NO PASSWORD REQUIRED!
        if (method == "Public Key") {
            accept();
            return;
        }

        // 2. If user chooses "Password" or "Keyboard Interactive", and password is empty: prompt
        if ((method == "Password" || method == "Keyboard Interactive") && m_password.isEmpty()) {
            bool ok = false;
            QString pwd = QInputDialog::getText(this, "Password Authentication",
                                                QString("Enter password for %1@%2:").arg(m_userEdit->text()).arg(m_hostEdit->text()),
                                                QLineEdit::Password, "", &ok);
            if (!ok) return;
            m_password = pwd;
        }

        // 3. If "<Profile Settings>": check if default private key exists
        if (method == "<Profile Settings>" && m_password.isEmpty()) {
            QString home = QDir::homePath();
            bool hasKey = QFile::exists(home + "/.ssh/id_rsa") ||
                          QFile::exists(home + "/.ssh/id_ed25519") ||
                          QFile::exists(home + "/.ssh/id_ecdsa");
            // If has key, connect directly using Public Key without password prompt!
            if (!hasKey) {
                bool ok = false;
                QString pwd = QInputDialog::getText(this, "Password Authentication",
                                                    QString("Enter password for %1@%2:").arg(m_userEdit->text()).arg(m_hostEdit->text()),
                                                    QLineEdit::Password, "", &ok);
                if (!ok) return;
                m_password = pwd;
            }
        }

        accept();
    });

    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    onHostTextChanged(m_hostEdit->text());
}

void QuickConnectDialog::onHostTextChanged(const QString &text) {
    m_connectBtn->setEnabled(!text.trimmed().isEmpty());
}

SshConnectionParams QuickConnectDialog::getParams() const {
    SshConnectionParams p;
    p.host = m_hostEdit->text().trimmed();
    p.username = m_userEdit->text().trimmed();
    p.port = m_portEdit->text().toInt();
    p.password = m_password;
    p.authMethod = m_authCombo->currentText();
    return p;
}

void QuickConnectDialog::setParams(const SshConnectionParams &params) {
    m_hostEdit->setText(params.host);
    m_userEdit->setText(params.username);
    m_portEdit->setText(QString::number(params.port));
    m_password = params.password;
    int idx = m_authCombo->findText(params.authMethod);
    if (idx >= 0) m_authCombo->setCurrentIndex(idx);
    onHostTextChanged(m_hostEdit->text());
}
