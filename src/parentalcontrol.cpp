#include <QTimer>
#include "parentalcontrol.h"
#include "config.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QFrame>

namespace YTKids {

// ── PinDialog ─────────────────────────────────────────────────────────────────
PinDialog::PinDialog(QWidget* parent)
    : QDialog(parent, Qt::FramelessWindowHint | Qt::Dialog)
{
    setModal(true);
    setFixedSize(320, 460);
    setStyleSheet(R"(
        QDialog {
            background-color: #1A0A35;
            border: 2px solid #3B2265;
            border-radius: 20px;
        }
        QPushButton {
            background-color: #2D1654;
            color: #FAFAFA;
            border: none;
            border-radius: 14px;
            font-size: 22px;
            font-weight: bold;
            min-width: 72px;
            min-height: 72px;
        }
        QPushButton:hover  { background-color: #3B2265; }
        QPushButton:pressed { background-color: #FFD166; color: #1A0A35; }
        QPushButton#backspace {
            background-color: #3B2265;
            font-size: 16px;
        }
        QLabel#title {
            color: #FAFAFA;
            font-size: 18px;
            font-weight: bold;
        }
        QLabel#dots {
            color: #FFD166;
            font-size: 32px;
            letter-spacing: 12px;
        }
        QLabel#error {
            color: #FF6B6B;
            font-size: 13px;
        }
    )");

    auto* root = new QVBoxLayout(this);
    root->setSpacing(16);
    root->setContentsMargins(28, 28, 28, 28);

    auto* title = new QLabel("🔐  Parental Controls", this);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    root->addWidget(title);

    m_dots = new QLabel("○ ○ ○ ○", this);
    m_dots->setObjectName("dots");
    m_dots->setAlignment(Qt::AlignCenter);
    root->addWidget(m_dots);

    buildNumpad();
}

void PinDialog::buildNumpad()
{
    auto* grid = new QGridLayout();
    grid->setSpacing(10);

    const QStringList digits = {
        "1","2","3",
        "4","5","6",
        "7","8","9",
        "","0","⌫"
    };

    int row = 0, col = 0;
    for (const QString& d : digits) {
        if (d.isEmpty()) {
            ++col;
            if (col > 2) { col = 0; ++row; }
            continue;
        }

        auto* btn = new QPushButton(d, this);
        if (d == "⌫") {
            btn->setObjectName("backspace");
            connect(btn, &QPushButton::clicked, this, &PinDialog::onBackspace);
        } else {
            connect(btn, &QPushButton::clicked, this, [this, d]() {
                onDigit(d);
            });
        }

        grid->addWidget(btn, row, col);
        ++col;
        if (col > 2) { col = 0; ++row; }
    }

    static_cast<QVBoxLayout*>(layout())->addLayout(grid);
}

void PinDialog::onDigit(const QString& digit)
{
    if (m_input.length() >= 4) return;
    m_input += digit;
    updateDots();

    if (m_input.length() == 4)
        onAccept();
}

void PinDialog::onBackspace()
{
    if (!m_input.isEmpty()) {
        m_input.chop(1);
        updateDots();
    }
}

void PinDialog::updateDots()
{
    const int filled = m_input.length();
    QString dots;
    for (int i = 0; i < 4; ++i) {
        dots += (i < filled) ? "●" : "○";
        if (i < 3) dots += " ";
    }
    m_dots->setText(dots);
}

void PinDialog::onAccept()
{
    Config& cfg = Config::instance();

    if (!cfg.hasPin() || cfg.verifyPin(m_input)) {
        m_verified = true;
        accept();
    } else {
        m_input.clear();
        updateDots();
        shake();
    }
}

void PinDialog::shake()
{
    auto* anim = new QSequentialAnimationGroup(this);
    const QPoint origin = pos();

    for (int i = 0; i < 4; ++i) {
        auto* left  = new QPropertyAnimation(this, "pos");
        left->setDuration(40);
        left->setStartValue(origin);
        left->setEndValue(origin + QPoint(-12, 0));

        auto* right = new QPropertyAnimation(this, "pos");
        right->setDuration(40);
        right->setStartValue(origin + QPoint(-12, 0));
        right->setEndValue(origin + QPoint(12, 0));

        anim->addAnimation(left);
        anim->addAnimation(right);
    }

    auto* reset = new QPropertyAnimation(this, "pos");
    reset->setDuration(40);
    reset->setStartValue(pos());
    reset->setEndValue(origin);
    anim->addAnimation(reset);

    anim->start(QAbstractAnimation::DeleteWhenStopped);

    m_dots->setStyleSheet("color: #FF6B6B; font-size: 32px; letter-spacing: 12px;");
    QTimer::singleShot(400, this, [this]() {
        m_dots->setStyleSheet("color: #FFD166; font-size: 32px; letter-spacing: 12px;");
    });
}

// ── ParentalControlDialog ─────────────────────────────────────────────────────
ParentalControlDialog::ParentalControlDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Parental Controls");
    setMinimumSize(560, 520);
    setStyleSheet(R"(
        QDialog, QWidget {
            background-color: #1A0A35;
            color: #FAFAFA;
            font-size: 14px;
        }
        QTabWidget::pane {
            border: 1px solid #3B2265;
            border-radius: 10px;
            background: #2D1654;
        }
        QTabBar::tab {
            background: #1A0A35;
            color: #A08BC0;
            padding: 8px 18px;
            border-radius: 8px 8px 0 0;
        }
        QTabBar::tab:selected {
            background: #2D1654;
            color: #FFD166;
            font-weight: bold;
        }
        QGroupBox {
            border: 1px solid #3B2265;
            border-radius: 10px;
            margin-top: 12px;
            padding: 10px;
            color: #A08BC0;
        }
        QSpinBox, QTimeEdit, QLineEdit {
            background: #1A0A35;
            color: #FAFAFA;
            border: 1px solid #3B2265;
            border-radius: 8px;
            padding: 6px 10px;
            font-size: 14px;
        }
        QCheckBox { color: #FAFAFA; }
        QCheckBox::indicator {
            width: 18px; height: 18px;
            border: 2px solid #A08BC0;
            border-radius: 4px;
        }
        QCheckBox::indicator:checked {
            background: #FFD166;
            border-color: #FFD166;
        }
        QListWidget {
            background: #1A0A35;
            border: 1px solid #3B2265;
            border-radius: 8px;
            color: #FAFAFA;
        }
        QListWidget::item:selected {
            background: #3B2265;
            color: #FFD166;
        }
        QPushButton {
            background: #FFD166;
            color: #1A0A35;
            border: none;
            border-radius: 10px;
            padding: 8px 18px;
            font-weight: bold;
        }
        QPushButton:hover { background: #ffe08a; }
        QPushButton#danger {
            background: #FF6B6B;
            color: white;
        }
        QPushButton#secondary {
            background: #3B2265;
            color: #FAFAFA;
        }
        QPushButton#secondary:hover { background: #4a2d7a; }
    )");

    auto* root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(20, 20, 20, 20);

    m_tabs = new QTabWidget(this);

    auto* sessionTab  = new QWidget();
    auto* scheduleTab = new QWidget();
    auto* contentTab  = new QWidget();
    auto* artworkTab  = new QWidget();
    auto* securityTab = new QWidget();

    buildSessionTab(sessionTab);
    buildScheduleTab(scheduleTab);
    buildContentTab(contentTab);
    buildArtworkTab(artworkTab);
    buildSecurityTab(securityTab);

    m_tabs->addTab(sessionTab,  "⏱  Session");
    m_tabs->addTab(scheduleTab, "📅  Schedule");
    m_tabs->addTab(contentTab,  "📺  Channels");
    m_tabs->addTab(artworkTab,  "🎨  Artwork");
    m_tabs->addTab(securityTab, "🔐  Security");

    root->addWidget(m_tabs);

    // ── Bottom buttons ────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();

    auto* resetBtn = new QPushButton("Reset to defaults");
    resetBtn->setObjectName("danger");
    connect(resetBtn, &QPushButton::clicked, this, &ParentalControlDialog::onReset);

    auto* saveBtn = new QPushButton("Save");
    connect(saveBtn, &QPushButton::clicked, this, &ParentalControlDialog::onSave);

    auto* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setObjectName("secondary");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnRow->addWidget(resetBtn);
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(saveBtn);
    root->addLayout(btnRow);

    loadFromConfig();
}

void ParentalControlDialog::buildSessionTab(QWidget* tab)
{
    auto* lay  = new QVBoxLayout(tab);
    auto* form = new QFormLayout();
    form->setSpacing(14);

    m_sessionSpin = new QSpinBox();
    m_sessionSpin->setRange(5, 180);
    m_sessionSpin->setSuffix("  min");
    form->addRow("Watch time:", m_sessionSpin);

    m_graceSpin = new QSpinBox();
    m_graceSpin->setRange(1, 30);
    m_graceSpin->setSuffix("  min");
    form->addRow("Finish video if under:", m_graceSpin);

    m_cooldownSpin = new QSpinBox();
    m_cooldownSpin->setRange(5, 180);
    m_cooldownSpin->setSuffix("  min");
    form->addRow("Cooldown after session:", m_cooldownSpin);

    auto* hint = new QLabel(
        "After the session ends, a cooldown period begins.\n"
        "No video is allowed until it completes."
    );
    hint->setStyleSheet("color: #A08BC0; font-size: 12px;");
    hint->setWordWrap(true);

    lay->addLayout(form);
    lay->addSpacing(10);
    lay->addWidget(hint);
    lay->addStretch();
}

void ParentalControlDialog::buildScheduleTab(QWidget* tab)
{
    auto* lay  = new QVBoxLayout(tab);
    auto* form = new QFormLayout();
    form->setSpacing(14);

    m_scheduleCheck = new QCheckBox("Enable time window");
    form->addRow(m_scheduleCheck);

    m_startTime = new QTimeEdit();
    m_startTime->setDisplayFormat("HH:mm");
    form->addRow("Allowed from:", m_startTime);

    m_endTime = new QTimeEdit();
    m_endTime->setDisplayFormat("HH:mm");
    form->addRow("Allowed until:", m_endTime);

    connect(m_scheduleCheck, &QCheckBox::toggled, m_startTime, &QWidget::setEnabled);
    connect(m_scheduleCheck, &QCheckBox::toggled, m_endTime,   &QWidget::setEnabled);

    auto* hint = new QLabel(
        "Outside the allowed window the app shows a friendly\n"
        "countdown until the next allowed time."
    );
    hint->setStyleSheet("color: #A08BC0; font-size: 12px;");
    hint->setWordWrap(true);

    lay->addLayout(form);
    lay->addSpacing(10);
    lay->addWidget(hint);
    lay->addStretch();
}

void ParentalControlDialog::buildContentTab(QWidget* tab)
{
    auto* lay = new QVBoxLayout(tab);

    // Allowed channels
    auto* allowGroup = new QGroupBox("Favourites (allowed channels)");
    auto* allowLay   = new QVBoxLayout(allowGroup);

    m_allowedChannels = new QListWidget();
    m_allowedChannels->setMaximumHeight(120);
    allowLay->addWidget(m_allowedChannels);

    auto* allowRow = new QHBoxLayout();
    m_channelInput = new QLineEdit();
    m_channelInput->setPlaceholderText("Channel name or youtube.com/channel/UC...");
    auto* addAllowBtn = new QPushButton("+ Allow");
    auto* remAllowBtn = new QPushButton("Remove");
    remAllowBtn->setObjectName("secondary");

    connect(addAllowBtn, &QPushButton::clicked,
            this, &ParentalControlDialog::onAddAllowedChannel);
    connect(remAllowBtn, &QPushButton::clicked,
            this, &ParentalControlDialog::onRemoveAllowedChannel);

    allowRow->addWidget(m_channelInput);
    allowRow->addWidget(addAllowBtn);
    allowRow->addWidget(remAllowBtn);
    allowLay->addLayout(allowRow);
    lay->addWidget(allowGroup);

    // Blocked channels
    auto* blockGroup = new QGroupBox("Blocked channels");
    auto* blockLay   = new QVBoxLayout(blockGroup);

    m_blockedChannels = new QListWidget();
    m_blockedChannels->setMaximumHeight(120);
    blockLay->addWidget(m_blockedChannels);

    auto* blockRow  = new QHBoxLayout();
    auto* blockInput = new QLineEdit();
    blockInput->setPlaceholderText("Channel name or youtube.com/channel/UC...");
    auto* addBlockBtn = new QPushButton("+ Block");
    auto* remBlockBtn = new QPushButton("Remove");
    remBlockBtn->setObjectName("secondary");

    connect(addBlockBtn, &QPushButton::clicked, this, [this, blockInput]() {
        const QString text = blockInput->text().trimmed();
        if (!text.isEmpty()) {
            m_blockedChannels->addItem(text);
            blockInput->clear();
        }
    });
    connect(remBlockBtn, &QPushButton::clicked,
            this, &ParentalControlDialog::onRemoveBlockedChannel);

    blockRow->addWidget(blockInput);
    blockRow->addWidget(addBlockBtn);
    blockRow->addWidget(remBlockBtn);
    blockLay->addLayout(blockRow);
    lay->addWidget(blockGroup);

    auto* hint = new QLabel(
        "Paste a YouTube channel URL or type a name.\n"
        "Blocked channels are hidden in the player."
    );
    hint->setStyleSheet("color: #A08BC0; font-size: 12px;");
    lay->addWidget(hint);
    lay->addStretch();
}

void ParentalControlDialog::buildArtworkTab(QWidget* tab)
{
    auto* lay  = new QVBoxLayout(tab);
    auto* form = new QFormLayout();
    form->setSpacing(14);

    m_artworkFolder = new QLineEdit();
    m_artworkFolder->setPlaceholderText("/home/user/Pictures");
    m_artworkFolder->setReadOnly(true);

    auto* browseBtn = new QPushButton("Browse…");
    browseBtn->setObjectName("secondary");
    connect(browseBtn, &QPushButton::clicked,
            this, &ParentalControlDialog::onBrowseArtworkFolder);

    auto* folderRow = new QHBoxLayout();
    folderRow->addWidget(m_artworkFolder);
    folderRow->addWidget(browseBtn);
    form->addRow("Artwork folder:", folderRow);

    auto* hint = new QLabel(
        "Your child can pick any image from this folder\n"
        "as the wallpaper for the waiting and locked screens.\n\n"
        "Tip: share your art folder here so she can choose\n"
        "from your work. 🎨"
    );
    hint->setStyleSheet("color: #A08BC0; font-size: 12px;");
    hint->setWordWrap(true);

    lay->addLayout(form);
    lay->addSpacing(10);
    lay->addWidget(hint);
    lay->addStretch();
}

void ParentalControlDialog::buildSecurityTab(QWidget* tab)
{
    auto* lay  = new QVBoxLayout(tab);
    auto* form = new QFormLayout();
    form->setSpacing(14);

    m_newPin = new QLineEdit();
    m_newPin->setEchoMode(QLineEdit::Password);
    m_newPin->setMaxLength(4);
    m_newPin->setPlaceholderText("4 digits");
    form->addRow("New PIN:", m_newPin);

    m_confirmPin = new QLineEdit();
    m_confirmPin->setEchoMode(QLineEdit::Password);
    m_confirmPin->setMaxLength(4);
    m_confirmPin->setPlaceholderText("Repeat PIN");
    form->addRow("Confirm PIN:", m_confirmPin);

    auto* setPinBtn = new QPushButton("Set PIN");
    connect(setPinBtn, &QPushButton::clicked,
            this, &ParentalControlDialog::onChangePin);

    auto* clearPinBtn = new QPushButton("Clear PIN");
    clearPinBtn->setObjectName("danger");
    connect(clearPinBtn, &QPushButton::clicked, this, []() {
        Config::instance().clearPin();
        QMessageBox::information(nullptr, "PIN cleared", "Parental PIN has been removed.");
    });

    auto* pinRow = new QHBoxLayout();
    pinRow->addWidget(setPinBtn);
    pinRow->addWidget(clearPinBtn);

    auto* hint = new QLabel(
        "Set a 4-digit PIN to protect parental settings.\n"
        "Leave blank to disable PIN protection."
    );
    hint->setStyleSheet("color: #A08BC0; font-size: 12px;");
    hint->setWordWrap(true);

    lay->addLayout(form);
    lay->addLayout(pinRow);
    lay->addSpacing(10);
    lay->addWidget(hint);
    lay->addStretch();
}

// ── Load / Save ───────────────────────────────────────────────────────────────
void ParentalControlDialog::loadFromConfig()
{
    Config& cfg = Config::instance();

    m_sessionSpin->setValue(cfg.sessionDuration()  / 60);
    m_graceSpin->setValue(cfg.gracePeriod()         / 60);
    m_cooldownSpin->setValue(cfg.cooldownDuration() / 60);

    m_scheduleCheck->setChecked(cfg.scheduleEnabled());
    m_startTime->setTime(cfg.allowedStart());
    m_endTime->setTime(cfg.allowedEnd());
    m_startTime->setEnabled(cfg.scheduleEnabled());
    m_endTime->setEnabled(cfg.scheduleEnabled());

    m_artworkFolder->setText(cfg.artworkFolder());

    m_allowedChannels->clear();
    for (const QString& ch : cfg.allowedChannels())
        m_allowedChannels->addItem(ch);

    m_blockedChannels->clear();
    for (const QString& ch : cfg.blockedChannels())
        m_blockedChannels->addItem(ch);
}

void ParentalControlDialog::saveToConfig()
{
    Config& cfg = Config::instance();

    cfg.setSessionDuration(m_sessionSpin->value()  * 60);
    cfg.setGracePeriod(m_graceSpin->value()         * 60);
    cfg.setCooldownDuration(m_cooldownSpin->value() * 60);

    cfg.setScheduleEnabled(m_scheduleCheck->isChecked());
    cfg.setAllowedStart(m_startTime->time());
    cfg.setAllowedEnd(m_endTime->time());

    cfg.setArtworkFolder(m_artworkFolder->text());

    QStringList allowed;
    for (int i = 0; i < m_allowedChannels->count(); ++i)
        allowed << m_allowedChannels->item(i)->text();
    cfg.setAllowedChannels(allowed);

    QStringList blocked;
    for (int i = 0; i < m_blockedChannels->count(); ++i)
        blocked << m_blockedChannels->item(i)->text();
    cfg.setBlockedChannels(blocked);
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void ParentalControlDialog::onSave()
{
    saveToConfig();
    accept();
}

void ParentalControlDialog::onReset()
{
    auto reply = QMessageBox::question(
        this, "Reset",
        "Reset all settings to defaults?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply == QMessageBox::Yes) {
        Config::instance().resetToDefaults();
        loadFromConfig();
    }
}

void ParentalControlDialog::onChangePin()
{
    const QString pin     = m_newPin->text().trimmed();
    const QString confirm = m_confirmPin->text().trimmed();

    if (pin.length() != 4 || !pin.toInt()) {
        QMessageBox::warning(this, "PIN error", "PIN must be 4 digits.");
        return;
    }
    if (pin != confirm) {
        QMessageBox::warning(this, "PIN error", "PINs do not match.");
        return;
    }

    Config::instance().setPin(pin);
    m_newPin->clear();
    m_confirmPin->clear();
    QMessageBox::information(this, "PIN set", "Parental PIN updated.");
}

void ParentalControlDialog::onAddAllowedChannel()
{
    const QString text = m_channelInput->text().trimmed();
    if (!text.isEmpty()) {
        m_allowedChannels->addItem(text);
        m_channelInput->clear();
    }
}

void ParentalControlDialog::onRemoveAllowedChannel()
{
    delete m_allowedChannels->takeItem(m_allowedChannels->currentRow());
}

void ParentalControlDialog::onAddBlockedChannel()
{
    // handled inline via lambda in buildContentTab
}

void ParentalControlDialog::onRemoveBlockedChannel()
{
    delete m_blockedChannels->takeItem(m_blockedChannels->currentRow());
}

void ParentalControlDialog::onBrowseArtworkFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select artwork folder",
        m_artworkFolder->text()
    );
    if (!dir.isEmpty())
        m_artworkFolder->setText(dir);
}

// ── Gate function ─────────────────────────────────────────────────────────────
bool openParentalControls(QWidget* parent)
{
    Config& cfg = Config::instance();

    if (cfg.hasPin()) {
        PinDialog pin(parent);
        pin.exec();
        if (!pin.verified())
            return false;
    }

    ParentalControlDialog dlg(parent);
    dlg.exec();
    return true;
}

} // namespace YTKids
