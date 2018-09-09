#include "governancelist.h"
#include "ui_governancelist.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "governance.h"
#include "governance-vote.h"
#include "governance-classes.h"
#include "governance-validators.h"
#include "messagesigner.h"
#include "clientmodel.h"
#include "../validation.h"
#include "../uint256.h"
#include "governancedialog.h"
#include "guiutil.h"

#include <QTimer>
#include <QMessageBox>
#include <rpc/protocol.h>


GovernanceList::GovernanceList(const PlatformStyle *platformStyle, QWidget *parent) :
        QWidget(parent),
        ui(new Ui::GovernanceList),
        clientModel(0),
        walletModel(0)
{
    ui->setupUi(this);
    int columnHash = 50;
    int columnName = 250;
    int columnUrl = 250;
    int columnAmount = 120;
    int columnvoteYes = 80;
    int columnvoteNo = 80;
    int columnAbsoluteYes = 150;
    int columnFund = 50;

    ui->tableWidgetGobjects->setColumnWidth(0, columnHash);
    ui->tableWidgetGobjects->setColumnWidth(1, columnName);
    ui->tableWidgetGobjects->setColumnWidth(2, columnUrl);
    ui->tableWidgetGobjects->setColumnWidth(3, columnAmount);
    ui->tableWidgetGobjects->setColumnWidth(4, columnvoteYes);
    ui->tableWidgetGobjects->setColumnWidth(5, columnvoteNo);
    ui->tableWidgetGobjects->setColumnWidth(6, columnAbsoluteYes);
    ui->tableWidgetGobjects->setColumnWidth(7, columnFund);


    contextMenu = new QMenu();
    connect(ui->tableWidgetGobjects, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(ui->tableWidgetGobjects, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_GovernanceButton_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateGobjects()));
    timer->start(1000);

    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
}

void GovernanceList::on_voteYesButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if(selected.count() == 0) return;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();

    }
    uint256 parsedGobjectHash;
    parsedGobjectHash.SetHex(gobjectSingle);
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm YES vote"),
                                                               tr("Are you sure you want vote YES on this proposal with all your masternodes?"),
                                                               QMessageBox::Yes | QMessageBox::Cancel,
                                                               QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    std::string strVoteSignal =  "funding";
    std::string strVoteOutcome = "yes";
    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
    if (eVoteSignal == VOTE_SIGNAL_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Invalid vote signal. Please using one of the following: "
                           "(funding|valid|delete|endorsed)");
    }

    vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
    if (eVoteOutcome == VOTE_OUTCOME_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
    }
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) return; // Unlock wallet was cancelled
        Vote(masternodeConfig.getEntries(), parsedGobjectHash, eVoteSignal, eVoteOutcome);
        return;
    }
    Vote(masternodeConfig.getEntries(), parsedGobjectHash, eVoteSignal, eVoteOutcome );
}

void GovernanceList::on_voteNoButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if(selected.count() == 0) return;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();

    }
    uint256 parsedGobjectHash;
    parsedGobjectHash.SetHex(gobjectSingle);
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm NO vote"),
                                                               tr("Are you sure you want vote NO on this proposal with all your masternodes?"),
                                                               QMessageBox::Yes | QMessageBox::Cancel,
                                                               QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;
    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    std::string strVoteSignal =  "funding";
    std::string strVoteOutcome = "no";
    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
    if (eVoteSignal == VOTE_SIGNAL_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Invalid vote signal. Please using one of the following: "
                           "(funding|valid|delete|endorsed)");
    }

    vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
    if (eVoteOutcome == VOTE_OUTCOME_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
    }
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) return; // Unlock wallet was cancelled
        Vote(masternodeConfig.getEntries(), parsedGobjectHash, eVoteSignal, eVoteOutcome);
        return;
    }
    Vote(masternodeConfig.getEntries(), parsedGobjectHash, eVoteSignal, eVoteOutcome );
}

void GovernanceList::on_voteAbstainButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();
        if(selected.count() == 0) return;
        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();

    }
    uint256 parsedGobjectHash;
    parsedGobjectHash.SetHex(gobjectSingle);
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm ABSTAIN vote"),
                                                               tr("Are you sure you want vote ABSTAIN on this proposal with all your masternodes?"),
                                                               QMessageBox::Yes | QMessageBox::Cancel,
                                                               QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();
    std::string strVoteSignal =  "funding";
    std::string strVoteOutcome = "abstain";
    vote_signal_enum_t eVoteSignal = CGovernanceVoting::ConvertVoteSignal(strVoteSignal);
    if (eVoteSignal == VOTE_SIGNAL_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           "Invalid vote signal. Please using one of the following: "
                           "(funding|valid|delete|endorsed)");
    }

    vote_outcome_enum_t eVoteOutcome = CGovernanceVoting::ConvertVoteOutcome(strVoteOutcome);
    if (eVoteOutcome == VOTE_OUTCOME_NONE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid vote outcome. Please use one of the following: 'yes', 'no' or 'abstain'");
    }
    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());
        if(!ctx.isValid()) return; // Unlock wallet was cancelled
        Vote(masternodeConfig.getEntries(), parsedGobjectHash, eVoteSignal, eVoteOutcome);
        return;
    }
    Vote(masternodeConfig.getEntries(), parsedGobjectHash, eVoteSignal, eVoteOutcome );
}

void GovernanceList::Vote(const std::vector<CMasternodeConfig::CMasternodeEntry>& entries,
                          const uint256& hash, vote_signal_enum_t eVoteSignal,
                          vote_outcome_enum_t eVoteOutcome)
{
    int nSuccessful = 0;
    int nFailed = 0;

    UniValue resultsObj(UniValue::VOBJ);

    for (const auto& mne : entries) {
        CPubKey pubKeyOperator;
        CKey keyOperator;

        UniValue statusObj(UniValue::VOBJ);

        if (!CMessageSigner::GetKeysFromSecret(mne.getPrivKey(), keyOperator, pubKeyOperator)) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Masternode signing error, could not set key correctly"));
            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
            continue;
        }

        uint256 nTxHash;
        nTxHash.SetHex(mne.getTxHash());

        int nOutputIndex = 0;
        if (!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        COutPoint outpoint(nTxHash, nOutputIndex);

        CMasternode mn;
        bool fMnFound = mnodeman.Get(outpoint, mn);

        if (!fMnFound) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Can't find masternode by collateral output"));
            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
            continue;
        }

        CGovernanceVote vote(mn.outpoint, hash, eVoteSignal, eVoteOutcome);
        if (!vote.Sign(keyOperator, pubKeyOperator.GetID())) {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Failure to sign."));
            resultsObj.push_back(Pair(mne.getAlias(), statusObj));
            continue;
        }

        CGovernanceException exception;
        if (governance.ProcessVoteAndRelay(vote, exception, *g_connman)) {
            nSuccessful++;
            statusObj.push_back(Pair("result", "success"));
        } else {
            nFailed++;
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", exception.GetMessage()));
        }

        resultsObj.push_back(Pair(mne.getAlias(), statusObj));
    }

    std::string returnObj;
    returnObj = strprintf("Successfully vote %d masternodes, failed to vote %d, total %d", nSuccessful, nFailed,nFailed + nSuccessful);
    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();
    update();
}

GovernanceList::~GovernanceList()
{
    delete ui;
}
void GovernanceList::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void GovernanceList::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetGobjects->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void GovernanceList::updateGobjects()
{
    TRY_LOCK(cs_gobjlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }

    static int64_t nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in GOBJECT_UPDATE_SECONDS seconds
    // or GOBJECT_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                            ? nTimeFilterUpdated - GetTime() + GOBJECT_COOLDOWN_SECONDS
                            : nTimeListUpdated - GetTime() + GOBJECT_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countGobjectLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(!fFilterUpdated) ui->countGobjectLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));

    if(nSecondsToWait > 0) return;

    nTimeListUpdated = GetTime();
    fFilterUpdated = false;

    QString strToFilter;
    ui->countGobjectLabel->setText("Updating...");
    ui->tableWidgetGobjects->setSortingEnabled(false);
    ui->tableWidgetGobjects->clearContents();
    ui->tableWidgetGobjects->setRowCount(0);


    int nStartTime = 0; //All
    std::vector<const CGovernanceObject*> objs = governance.GetAllNewerThan(nStartTime);

    for (const auto& pGovObj : objs)
    {
        int gobject = pGovObj->GetObjectType();

        if (gobject == 1) {
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key


            // Get Object as Hex and convert to std::string
            std::string HexStr = pGovObj->GetDataAsHexString();
            std::vector<unsigned char> v = ParseHex(HexStr);
            std::string s(v.begin(), v.end());
            std::string nameStr = getValue(s, "name", true);
            std::string urlStr = getValue(s, "url", true);
            std::string amountStr = getNumericValue(s, "payment_amount");


            // Define "Funding" for Vote count
            vote_signal_enum_t VoteCountType = vote_signal_enum_t(1);

            std::string vFunding;
            if (pGovObj->IsSetCachedFunding()) {
                vFunding = "Yes";
            } else {vFunding = "No";}

            QString name =  QString::fromStdString(nameStr);
            QString url = QString::fromStdString(urlStr);
            QString amount = QString::fromStdString(amountStr);
            std::string hash = pGovObj->GetHash().GetHex();

        QTableWidgetItem *Hash = new QTableWidgetItem(QString::fromStdString(hash));
        QTableWidgetItem *nameItem = new QTableWidgetItem(name);
        QTableWidgetItem *urlItem = new QTableWidgetItem(url);
        QTableWidgetItem *amounItem = new QTableWidgetItem(amount);
        QTableWidgetItem *voteYes = new QTableWidgetItem(QString::number(pGovObj->GetYesCount(VoteCountType)));
        QTableWidgetItem *voteNo = new QTableWidgetItem(QString::number(pGovObj->GetNoCount(VoteCountType)));
        QTableWidgetItem *AbsoluteYes = new QTableWidgetItem(QString::number(pGovObj->GetAbsoluteYesCount(VoteCountType)));
        QTableWidgetItem *fundingStatus = new QTableWidgetItem(QString::fromStdString(vFunding));

       if (strCurrentFilter != "")
        {
            strToFilter =   Hash->text() + " " +
                            nameItem->text() + " " +
                            amounItem->text() + " " +
                            voteYes->text() + " " +
                            voteNo->text() + " " +
                            AbsoluteYes->text() + " " +
                            fundingStatus->text();
            if (!strToFilter.contains(strCurrentFilter)) continue;
        }

        ui->tableWidgetGobjects->insertRow(0);
        ui->tableWidgetGobjects->setItem(0, 0, Hash);
        ui->tableWidgetGobjects->setItem(0, 1, nameItem);
        ui->tableWidgetGobjects->setItem(0, 2, urlItem);
        ui->tableWidgetGobjects->setItem(0, 3, amounItem);
        ui->tableWidgetGobjects->setItem(0, 4, voteYes);
        ui->tableWidgetGobjects->setItem(0, 5, voteNo);
        ui->tableWidgetGobjects->setItem(0, 6, AbsoluteYes);
        ui->tableWidgetGobjects->setItem(0, 7, fundingStatus);
        }
    }

    ui->countGobjectLabel->setText(QString::number(ui->tableWidgetGobjects->rowCount()));
    ui->tableWidgetGobjects->setSortingEnabled(true);
}

void GovernanceList::setClientModel(ClientModel *model)
{
    this->clientModel = model;
}

void GovernanceList::on_UpdateButton_clicked()
{
    updateGobjects();
}
void GovernanceList::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->countGobjectLabel->setText(QString::fromStdString(strprintf("Please wait... %d", GOBJECT_UPDATE_SECONDS)));
}

void GovernanceList::on_GovernanceButton_clicked()
{
    std::string gobjectSingle;
    {
        LOCK(cs_gobjlist);
        // Find selected gobject
        QItemSelectionModel* selectionModel = ui->tableWidgetGobjects->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if(selected.count() == 0) return;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        gobjectSingle = ui->tableWidgetGobjects->item(nSelectedRow, 0)->text().toStdString();

    }
         uint256 parsedGobjectHash;
         parsedGobjectHash.SetHex(gobjectSingle);
         ShowGovernanceObject(parsedGobjectHash);
}

void GovernanceList::ShowGovernanceObject(uint256 gobjectSingle) {

    if(!walletModel || !walletModel->getOptionsModel())
        return;

    CGovernanceObject* pGovObj = governance.FindGovernanceObject(gobjectSingle);

    // Title of popup window
    QString strWindowtitle = tr("Additional information for Governance Object");

    // Title above QR-Code
    QString strQRCodeTitle = "Governance Object";
    vote_signal_enum_t VotesFunding = vote_signal_enum_t(1);
    vote_signal_enum_t VotesValid = vote_signal_enum_t(2);
    vote_signal_enum_t VotesDelete = vote_signal_enum_t(3);
    vote_signal_enum_t VotesEndorsed = vote_signal_enum_t(4);


    std::string s = pGovObj->GetDataAsPlainString();
    std::string dataString = pGovObj->GetDataAsPlainString();
    std::string dataHex = pGovObj->GetDataAsHexString();
    std::string name = getValue(s,"name", true);
    std::string url = getValue(s,"url", true);
    std::string amount = getNumericValue(s,"payment_amount");
    std::string hash = gobjectSingle.ToString();
    // Funding Variables
    std::string FundingYes = std::to_string(pGovObj->GetYesCount(VotesFunding));
    std::string FundingNo = std::to_string(pGovObj->GetNoCount(VotesFunding));
    std::string FundingAbstain = std::to_string(pGovObj->GetAbstainCount(VotesFunding));
    std::string FundingAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesFunding));
    // Valid Variables
    std::string ValidYes = std::to_string(pGovObj->GetYesCount(VotesValid));
    std::string ValidNo = std::to_string(pGovObj->GetNoCount(VotesValid));
    std::string ValidAbstain = std::to_string(pGovObj->GetAbstainCount(VotesValid));
    std::string ValidAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesValid));
    // Delete Variables
    std::string DeleteYes = std::to_string(pGovObj->GetYesCount(VotesDelete));
    std::string DeleteNo = std::to_string(pGovObj->GetNoCount(VotesDelete));
    std::string DeleteAbstain = std::to_string(pGovObj->GetAbstainCount(VotesDelete));
    std::string DeleteAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesDelete));
    // Endorse Variables
    std::string EndorseYes = std::to_string(pGovObj->GetYesCount(VotesEndorsed));
    std::string EndorseNo = std::to_string(pGovObj->GetNoCount(VotesEndorsed));
    std::string EndorseAbstain = std::to_string(pGovObj->GetAbstainCount(VotesEndorsed));
    std::string EndorseAbYes = std::to_string(pGovObj->GetAbsoluteYesCount(VotesEndorsed));
    // Create dialog text as HTML
    QString strHTML = "<html><font face='verdana, arial, helvetica, sans-serif'>";
    strHTML += "<b>" + tr("Hash") +          ": </b>" + GUIUtil::HtmlEscape(hash) + "<br>";
    strHTML += "<b>" + tr("Proposal Name") +      ": </b>" + GUIUtil::HtmlEscape(name) + "<br>";
    strHTML += "<b>" + tr("Proposal Url") +       ": </b>" + GUIUtil::HtmlEscape(url) + "<br>";
    strHTML += "<b>" + tr("Payment Amount") +     ": </b>" + GUIUtil::HtmlEscape(amount) + "<br>";
    strHTML += "<p><b>" + tr("Funding Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(FundingYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(FundingNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(FundingAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(FundingAbYes) + "</span></p>";
    strHTML += "<p><b>" + tr("Valid Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(ValidYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(ValidNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(ValidAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(ValidAbYes) + "</span></p>";
    strHTML += "<p><b>" + tr("Delete Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(DeleteYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(DeleteNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(DeleteAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(DeleteAbYes) + "</span></p>";
    strHTML += "<p><b>" + tr("Endorse Votes") +":</b>" + "<br><span>Yes: " + GUIUtil::HtmlEscape(EndorseYes) + "</span>" + "<br><span>No: " + GUIUtil::HtmlEscape(EndorseNo) + "</span>" + "<br><span>Abstain: " + GUIUtil::HtmlEscape(EndorseAbstain) + "</span>" + "<br><span>Absolute Yes: " + GUIUtil::HtmlEscape(EndorseAbYes) + "</span></p>";
    strHTML += "<b>" + tr("Raw Information (Hex)") +     ": </b>" + GUIUtil::HtmlEscape(dataHex) + "<br>";
    strHTML += "<b>" + tr("Raw Information (String)") +     ": </b>" + GUIUtil::HtmlEscape(dataString) + "<br>";

    // Open Governance dialog
    GovernanceDialog *dialog = new GovernanceDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModel(walletModel->getOptionsModel());
    dialog->setInfo(strWindowtitle, strWindowtitle,strHTML, "");
    dialog->show();
}

std::string getValue(std::string str,std::string key, bool format) {
  std::string s_pattern = "\"" + key + "\"";

  int beg = str.find(s_pattern);
  int f_comma = str.find("\"", beg+s_pattern.size());
  int s_comma = str.find("\"", f_comma+1);
  std::string s2 = str.substr(f_comma+1, s_comma-f_comma-1);
  return s2;
}



std::string getNumericValue(std::string str, std::string key){
    std::string s_pattern = "\"" + key + "\"";

    int beg = str.find(s_pattern);
    int f_comma = str.find(":", beg+s_pattern.size());
    int s_comma = str.find(",", f_comma+1);
    std::string s2 = str.substr(f_comma+1, s_comma-f_comma-1);
    return s2;
}