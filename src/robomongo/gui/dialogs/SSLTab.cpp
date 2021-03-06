#include "robomongo/gui/dialogs/SSLTab.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QFileInfo>
#include <QFrame>
#include <QRadioButton>
#include <QSpacerItem>
#include <QHBoxLayout>

#include <mongo/util/net/ssl_options.h>
#include <mongo/util/net/ssl_manager.h>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SslSettings.h"


namespace Robomongo
{
    SSLTab::SSLTab(ConnectionSettings *settings) 
        : _settings(settings)
    {
        const SslSettings* const sslSettings = _settings->sslSettings();

        // Use SSL section
        _useSslCheckBox = new QCheckBox("Use SSL protocol");
        _useSslCheckBox->setStyleSheet("margin-bottom: 7px");
        _useSslCheckBox->setChecked(sslSettings->enabled());
        VERIFY(connect(_useSslCheckBox, SIGNAL(stateChanged(int)), this, SLOT(useSslCheckBoxStateChange(int))));

        // CA Section
        _acceptSelfSignedButton = new QRadioButton("Accept self-signed certificates");
        _useRootCaFileButton = new QRadioButton("Use Root CA file: ");
        _acceptSelfSignedButton->setChecked(sslSettings->allowInvalidCertificates());
        _useRootCaFileButton->setChecked(!_acceptSelfSignedButton->isChecked());
        //_caFileLabel = new QLabel("Root CA file: ");
        _caFilePathLineEdit = new QLineEdit;
        _caFileBrowseButton = new QPushButton("...");
        _caFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_caFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_caFileBrowseButton_clicked())));
        VERIFY(connect(_acceptSelfSignedButton, SIGNAL(toggled(bool)), this, SLOT(on_acceptSelfSignedButton_toggle(bool))));

        // Client Cert section
        _useClientCertCheckBox = new QCheckBox("Use Client Certificate ( --sslPemKeyFile )");
        _clientCertLabel = new QLabel("Client Certificate: ");
        _clientCertPathLineEdit = new QLineEdit;
        _clientCertFileBrowseButton = new QPushButton("...");
        _clientCertFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_clientCertFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_pemKeyFileBrowseButton_clicked())));

        _clientCertPassLabel = new QLabel("Passphrase: ");
        _clientCertPassLineEdit = new QLineEdit;
        _clientCertPassShowButton = new QPushButton("Show");
        togglePassphraseShowMode();
        VERIFY(connect(_clientCertPassShowButton, SIGNAL(clicked()), this, SLOT(togglePassphraseShowMode())));
        _useClientCertPassCheckBox = new QCheckBox("Client key is encrypted with passphrase");
        _useClientCertPassCheckBox->setChecked(sslSettings->pemKeyEncrypted());
        VERIFY(connect(_useClientCertPassCheckBox, SIGNAL(toggled(bool)), this, SLOT(on_useClientCertPassCheckBox_toggle(bool))));

        // Advanced options
        _allowInvalidHostnamesCheckBox = new QCheckBox("Allow connections to servers with non-matching hostnames");
        _allowInvalidHostnamesCheckBox->setChecked(sslSettings->allowInvalidHostnames());
        _crlFileLabel = new QLabel("CRL file: ");
        _crlFilePathLineEdit = new QLineEdit;
        _crlFileBrowseButton = new QPushButton("...");
        _crlFileBrowseButton->setMaximumWidth(50);
        VERIFY(connect(_crlFileBrowseButton, SIGNAL(clicked()), this, SLOT(on_crlFileBrowseButton_clicked())));

        // Layouts
        // CA section
        QGridLayout* gridLayout = new QGridLayout;
        gridLayout->addWidget(_acceptSelfSignedButton,          0 ,0, 1, 3);
        gridLayout->addWidget(_useRootCaFileButton,             1, 0);
        gridLayout->addWidget(_caFilePathLineEdit,              1, 1);
        gridLayout->addWidget(_caFileBrowseButton,              1, 2);        
        // Client Section
        gridLayout->addWidget(_clientCertLabel,                 2, 0);
        gridLayout->addWidget(_clientCertPathLineEdit,          2, 1);
        gridLayout->addWidget(_clientCertFileBrowseButton,      2, 2);
        gridLayout->addWidget(_clientCertPassLabel,             3, 0);
        gridLayout->addWidget(_clientCertPassLineEdit,          3, 1);
        gridLayout->addWidget(_clientCertPassShowButton,        3, 2);
        gridLayout->addWidget(_useClientCertPassCheckBox,       4, 1);
        //// Advanced section
        gridLayout->addWidget(_crlFileLabel,                    5, 0);
        gridLayout->addWidget(_crlFilePathLineEdit,             5, 1);
        gridLayout->addWidget(_crlFileBrowseButton,             5, 2);
        gridLayout->addWidget(_allowInvalidHostnamesCheckBox,   6, 0, 1, 3);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->setAlignment(Qt::AlignTop);
        mainLayout->addWidget(_useSslCheckBox);
        mainLayout->addWidget(new QLabel(""));
        mainLayout->addLayout(gridLayout);
        setLayout(mainLayout);

        // Update widgets according to settings
        useSslCheckBoxStateChange(_useSslCheckBox->checkState());
        _caFilePathLineEdit->setText(QString::fromStdString(sslSettings->caFile()));
        _clientCertPathLineEdit->setText(QString::fromStdString(sslSettings->pemKeyFile()));
        _clientCertPassLineEdit->setText(QString::fromStdString(sslSettings->pemPassPhrase()));
        _allowInvalidHostnamesCheckBox->setChecked(sslSettings->allowInvalidHostnames());
        _crlFilePathLineEdit->setText(QString::fromStdString(sslSettings->crlFile()));
    }

    void SSLTab::on_acceptSelfSignedButton_toggle(bool checked)
    {
        _useRootCaFileButton->setChecked(!checked);
        setDisabledCAfileWidgets(checked);
    }

    void SSLTab::on_caFileBrowseButton_clicked()
    {
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _caFilePathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName =  QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _caFilePathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }


        // todo: remove
        //mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_requireSSL);
        ////mongo::sslGlobalParams.sslPEMKeyFile = "C:\\cygwin\\etc\\ssl\\mongodb.pem";
        //mongo::sslGlobalParams.sslCAFile = "C:\\cygwin\\demarcek_pass\\mongodb.pem";
        //mongo::sslGlobalParams.sslPEMKeyFile = "C:\\cygwin\\demarcek_pass\\client.pem";
        //mongo::sslGlobalParams.sslPEMKeyPassword = "testc";

        //mongo::DBClientConnection *conn = new mongo::DBClientConnection(true, 10);
        ////mongo::Status status = conn->connect(mongo::HostAndPort("46.101.137.132", 27020));

        //try{
        //    mongo::Status status = conn->connect(mongo::HostAndPort("localhost", 27017));
        //    _clientCertPassLineEdit->setText("isOk: " + QString::number(status.isOK()));
        //}
        //catch (const std::exception& ex){
        //    _clientCertPassLineEdit->setText("exc: " + QString::fromStdString(ex.what()));
        //}

        //mongo::sslGlobalParams.sslMode.store(mongo::SSLParams::SSLMode_allowSSL);
    }

    void SSLTab::on_pemKeyFileBrowseButton_clicked()
    {
        // todo: move to function
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _clientCertPathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _clientCertPathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }

    void SSLTab::on_crlFileBrowseButton_clicked()
    {
        // todo: move to function
        // If user has previously selected a file, initialize file dialog
        // with that file's name; otherwise, use user's home directory.
        QString initialName = _crlFilePathLineEdit->text();
        if (initialName.isEmpty())
        {
            initialName = QDir::homePath();
        }
        QString fileName = QFileDialog::getOpenFileName(this, tr("Choose File"));
        fileName = QDir::toNativeSeparators(fileName);
        if (!fileName.isEmpty()) {
            _crlFilePathLineEdit->setText(fileName);
            //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }

    void SSLTab::useSslCheckBoxStateChange(int state)
    {
        bool isChecked = static_cast<bool>(state);
        _acceptSelfSignedButton->setDisabled(!isChecked);
        _useRootCaFileButton->setDisabled(!isChecked);
        if (state)  // if SSL enabled, disable/enable conditionally; otherwise disable all widgets.
        {
            setDisabledCAfileWidgets(_acceptSelfSignedButton->isChecked());
            on_useClientCertPassCheckBox_toggle(_useClientCertPassCheckBox->isChecked());
        }
        else
        {
            setDisabledCAfileWidgets(true);
            _clientCertPassLineEdit->setDisabled(true);
            _clientCertPassShowButton->setDisabled(true);
        }
        _clientCertLabel->setDisabled(!isChecked);
        _clientCertPathLineEdit->setDisabled(!isChecked);
        _clientCertFileBrowseButton->setDisabled(!isChecked);
        _clientCertPassLabel->setDisabled(!isChecked);
        _useClientCertCheckBox->setDisabled(!isChecked);
        _useClientCertPassCheckBox->setDisabled(!isChecked);
        _allowInvalidHostnamesCheckBox->setDisabled(!isChecked);
        _crlFileLabel->setDisabled(!isChecked);
        _crlFilePathLineEdit->setDisabled(!isChecked);
        _crlFileBrowseButton->setDisabled(!isChecked);
    }

    void SSLTab::togglePassphraseShowMode()
    {
        bool isPassword = _clientCertPassLineEdit->echoMode() == QLineEdit::Password;
        _clientCertPassLineEdit->setEchoMode(isPassword ? QLineEdit::Normal : QLineEdit::Password);
        _clientCertPassShowButton->setText(isPassword ? tr("Hide") : tr("Show"));
    }

    void SSLTab::on_useClientCertPassCheckBox_toggle(bool checked)
    {
        _clientCertPassLineEdit->setDisabled(!checked);
        _clientCertPassShowButton->setDisabled(!checked);
    }

    bool SSLTab::accept()
    {
        SslSettings *sslSettings = _settings->sslSettings();
        sslSettings->enableSSL(_useSslCheckBox->isChecked());
        sslSettings->setCaFile(QtUtils::toStdString(_caFilePathLineEdit->text()));
        sslSettings->setPemKeyFile(QtUtils::toStdString(_clientCertPathLineEdit->text()));
        sslSettings->setPemPassPhrase(QtUtils::toStdString(_clientCertPassLineEdit->text()));
        sslSettings->setPemKeyEncrypted(_useClientCertPassCheckBox->isChecked()) ;
        sslSettings->setAllowInvalidCertificates(_acceptSelfSignedButton->isChecked());
        sslSettings->setAllowInvalidHostnames(_allowInvalidHostnamesCheckBox->isChecked());
        sslSettings->setCrlFile(QtUtils::toStdString(_crlFilePathLineEdit->text()));
        return true;
    }

    void SSLTab::setDisabledCAfileWidgets(bool disabled)
    {
        //_caFileLabel->setDisabled(disabled);
        _caFilePathLineEdit->setDisabled(disabled);
        _caFileBrowseButton->setDisabled(disabled);
    }
}

