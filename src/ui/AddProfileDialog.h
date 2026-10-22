#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

class AddProfileDialog : public QDialog {
public:
    explicit AddProfileDialog(const QString &defaultName = "", QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Add Profile");
        resize(360, 65);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(8);

        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setText(defaultName.isEmpty() ? "Profile Name" : defaultName);
        m_nameEdit->selectAll();
        layout->addWidget(m_nameEdit, 1);

        m_addBtn = new QPushButton("Add to Profiles", this);
        m_addBtn->setDefault(true);
        connect(m_addBtn, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(m_addBtn);
    }

    QString profileName() const { return m_nameEdit->text().trimmed(); }

private:
    QLineEdit *m_nameEdit = nullptr;
    QPushButton *m_addBtn = nullptr;
};
