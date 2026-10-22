#pragma once

#include "backend/SshSessionWorker.h"
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>

class QuickConnectDialog : public QDialog {
    Q_OBJECT

public:
    explicit QuickConnectDialog(QWidget *parent = nullptr);
    SshConnectionParams getParams() const;
    void setParams(const SshConnectionParams &params);

private slots:
    void onHostTextChanged(const QString &text);

private:
    QLineEdit *m_hostEdit = nullptr;
    QLineEdit *m_userEdit = nullptr;
    QLineEdit *m_portEdit = nullptr;
    QComboBox *m_authCombo = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;

    QString m_password;
};
