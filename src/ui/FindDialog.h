#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

class FindDialog : public QDialog {
public:
    explicit FindDialog(QWidget *parent = nullptr)
        : QDialog(parent) {
        setWindowTitle("Find");
        resize(420, 150);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        QHBoxLayout *rootHl = new QHBoxLayout(this);
        rootHl->setContentsMargins(12, 12, 12, 12);
        rootHl->setSpacing(12);

        // Left area: Input line and Options
        QVBoxLayout *leftVl = new QVBoxLayout();
        leftVl->setSpacing(10);

        // Find what row
        QHBoxLayout *inputHl = new QHBoxLayout();
        inputHl->setSpacing(6);
        QLabel *lbl = new QLabel("Fi&nd what:", this);
        inputHl->addWidget(lbl);

        m_findEdit = new QLineEdit(this);
        lbl->setBuddy(m_findEdit);
        inputHl->addWidget(m_findEdit, 1);

        QPushButton *moreBtn = new QPushButton("...", this);
        moreBtn->setFixedWidth(24);
        inputHl->addWidget(moreBtn);
        leftVl->addLayout(inputHl);

        // Lower options area: Left checkboxes, Right Direction groupbox
        QHBoxLayout *optsHl = new QHBoxLayout();
        optsHl->setSpacing(12);

        QVBoxLayout *cbVl = new QVBoxLayout();
        cbVl->setSpacing(4);
        m_matchWordCb = new QCheckBox("Match &whole word only", this);
        m_matchCaseCb = new QCheckBox("Match &case", this);
        m_regexCb = new QCheckBox("Regular &expression", this);
        cbVl->addWidget(m_matchWordCb);
        cbVl->addWidget(m_matchCaseCb);
        cbVl->addWidget(m_regexCb);
        optsHl->addLayout(cbVl);

        // Direction GroupBox
        QGroupBox *dirGb = new QGroupBox("Direction", this);
        QVBoxLayout *dirVl = new QVBoxLayout(dirGb);
        dirVl->setContentsMargins(8, 4, 8, 6);
        dirVl->setSpacing(2);
        m_upRadio = new QRadioButton("&Up", dirGb);
        m_downRadio = new QRadioButton("&Down", dirGb);
        m_upRadio->setChecked(true); // Default searching backwards/upwards in terminal history
        dirVl->addWidget(m_upRadio);
        dirVl->addWidget(m_downRadio);
        optsHl->addWidget(dirGb);

        leftVl->addLayout(optsHl);
        rootHl->addLayout(leftVl, 1);

        // Right buttons (Find Next, Cancel)
        QVBoxLayout *btnVl = new QVBoxLayout();
        btnVl->setSpacing(6);

        m_findNextBtn = new QPushButton("Find &Next", this);
        m_findNextBtn->setDefault(true);
        m_findNextBtn->setFixedWidth(85);
        connect(m_findNextBtn, &QPushButton::clicked, this, [this]() {
            if (m_onFindNext) {
                m_onFindNext();
            }
        });
        btnVl->addWidget(m_findNextBtn);

        QPushButton *cancelBtn = new QPushButton("Cancel", this);
        cancelBtn->setFixedWidth(85);
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::close);
        btnVl->addWidget(cancelBtn);
        btnVl->addStretch();

        rootHl->addLayout(btnVl);
    }

    void setOnFindNext(const std::function<void()> &callback) {
        m_onFindNext = callback;
    }

    QString findWhat() const { return m_findEdit->text(); }
    void setFindWhat(const QString &text) {
        m_findEdit->setText(text);
        m_findEdit->selectAll();
    }
    bool matchWholeWord() const { return m_matchWordCb->isChecked(); }
    bool matchCase() const { return m_matchCaseCb->isChecked(); }
    bool isRegex() const { return m_regexCb->isChecked(); }
    bool isDirectionUp() const { return m_upRadio->isChecked(); }

private:
    QLineEdit *m_findEdit = nullptr;
    QCheckBox *m_matchWordCb = nullptr;
    QCheckBox *m_matchCaseCb = nullptr;
    QCheckBox *m_regexCb = nullptr;
    QRadioButton *m_upRadio = nullptr;
    QRadioButton *m_downRadio = nullptr;
    QPushButton *m_findNextBtn = nullptr;
    std::function<void()> m_onFindNext;
};
