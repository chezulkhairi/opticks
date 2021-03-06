/*
 * The information in this file is
 * Copyright(c) 2010 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtGui/QComboBox>
#include <QtGui/QDateEdit>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QTabWidget>
#include <QtGui/QTextEdit>

#include "AppConfig.h"
#include "ApplicationServices.h"
#include "AppVerify.h"
#include "ClassificationWidget.h"
#include "DateTime.h"
#include "InfoBar.h"
#include "ObjectFactory.h"
#include "ObjectResource.h"
#include "Slot.h"
#include "StringUtilities.h"
#include "UtilityServices.h"

using namespace std;

namespace
{
   string trimString(string text)
   {
      const char* pText = text.c_str();

      if (text.size() == 0)
      {
         return text;
      }

      unsigned int i;
      for (i = 0; i < text.size(); i++)
      {
         if (!isspace(pText[i]))
         {
            break;
         }
      }

      unsigned int j;
      for (j = text.size() - 1; j > 0; j--)
      {
         if (!isspace(pText[j]))
         {
            break;
         }
      }

      return text.substr(i, j + 1);
   }

};

template <class T>
void populateListFromFile(T* box, const QString& strFilename, bool addBlank)
{
   if ((box == NULL) || (strFilename.isEmpty() == true))
   {
      return;
   }

   string filename = strFilename.toStdString();

   FILE* stream = fopen(filename.c_str(), "r");
   if (stream != NULL)
   {
      box->clear();

      char buffer[1024];
      while (!feof(stream))
      {
         buffer[1023] = 0;
         if (fgets (buffer, 1023, stream) != NULL)
         {
            for (unsigned int i = 0; i < strlen(buffer); i++)
            {
               if (buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '/')
               {
                  buffer[i] = 0;
                  break;
               }
            }

            box->addItem(QString::fromStdString(trimString(buffer)));
         }
      }

      if (addBlank)
      {
         box->addItem(QString());
      }

      box->setCurrentIndex(-1);
      fclose(stream);
   }
}

ClassificationWidget::ClassificationWidget(QWidget* pParent) :
   QWidget(pParent),
   mpClass(NULL),
   mNeedsInitialization(false)
{
   // Classification level
   QLabel* pClassificationLabel = new QLabel("Classification Level:", this);
   mpClassLevelCombo = new QComboBox(this);
   mpClassLevelCombo->setEditable(false);

   // Favorites
   QLabel* pFavoritesLabel = new QLabel("Favorites:", this);
   mpFavoritesCombo = new QComboBox(this);
   mpFavoritesCombo->setEditable(false);

   // Top and bottom tab
   mpTabWidget = new QTabWidget(this);

   QWidget* pTopTab = new QWidget();
   QListWidget* pFieldList = new QListWidget(pTopTab);
   pFieldList->addItem("Codewords");
   pFieldList->addItem("System");
   pFieldList->addItem("Releasability");
   pFieldList->addItem("Country Code");
   pFieldList->addItem("Declassification");
   pFieldList->setFixedWidth(100);

   mpValueStack = new QStackedWidget(pTopTab);

   mpCodewordList = new QListWidget(mpValueStack);
   mpCodewordList->setSelectionMode(QAbstractItemView::ExtendedSelection);
   mpValueStack->addWidget(mpCodewordList);

   mpSystemList = new QListWidget(mpValueStack);
   mpSystemList->setSelectionMode(QAbstractItemView::ExtendedSelection);
   mpValueStack->addWidget(mpSystemList);

   mpFileReleasingList = new QListWidget(mpValueStack);
   mpFileReleasingList->setSelectionMode(QAbstractItemView::ExtendedSelection);
   mpValueStack->addWidget(mpFileReleasingList);

   mpCountryCodeList = new QListWidget(mpValueStack);
   mpCountryCodeList->setSelectionMode(QAbstractItemView::ExtendedSelection);
   mpValueStack->addWidget(mpCountryCodeList);

   mpExemptionList = new QListWidget(mpValueStack);
   mpExemptionList->setSelectionMode(QAbstractItemView::ExtendedSelection);
   mpValueStack->addWidget(mpExemptionList);

   InfoBar* pInfoBar = new InfoBar(pTopTab);
   pInfoBar->setBackgroundColor(Qt::gray);
   pInfoBar->setTitleColor(Qt::black);
#if !defined(LINUX)
   pInfoBar->setTitleFont(QFont("Tahoma", 8, QFont::Bold, true));
#endif
   connect(pFieldList, SIGNAL(currentTextChanged(const QString&)), pInfoBar, SLOT(setTitle(const QString&)));
   connect(pFieldList, SIGNAL(currentRowChanged(int)), mpValueStack, SLOT(setCurrentIndex(int)));

   mpAddButton = new QPushButton("&Add...", pTopTab);
   QPushButton* pResetButton = new QPushButton("&Reset", pTopTab);

   QHBoxLayout* pTopButtonLayout = new QHBoxLayout();
   pTopButtonLayout->setMargin(0);
   pTopButtonLayout->setSpacing(5);
   pTopButtonLayout->addStretch(10);
   pTopButtonLayout->addWidget(mpAddButton);
   pTopButtonLayout->addWidget(pResetButton);

   QLabel* pPreviewLabel = new QLabel("Preview:", pTopTab);
   mpMarkingsEdit = new QTextEdit(pTopTab);
   mpMarkingsEdit->setFrameStyle(QFrame::Panel | QFrame::Sunken);
   mpMarkingsEdit->setReadOnly(true);
   mpMarkingsEdit->setLineWrapMode(QTextEdit::WidgetWidth);
   mpMarkingsEdit->setFixedHeight(48);

   QFont markingsFont(mpMarkingsEdit->font());
   markingsFont.setBold(true);
   mpMarkingsEdit->setFont(markingsFont);

   QGridLayout* pTopGrid = new QGridLayout(pTopTab);
   pTopGrid->setMargin(10);
   pTopGrid->setSpacing(5);
   pTopGrid->addWidget(pFieldList, 0, 0, 3, 1);
   pTopGrid->addWidget(pInfoBar, 0, 1);
   pTopGrid->addWidget(mpValueStack, 1, 1);
   pTopGrid->addLayout(pTopButtonLayout, 2, 1);
   pTopGrid->addWidget(pPreviewLabel, 3, 0, 1, 2);
   pTopGrid->addWidget(mpMarkingsEdit, 4, 0, 1, 2);
   pTopGrid->setRowStretch(1, 10);
   pTopGrid->setColumnStretch(1, 10);

   mpTabWidget->addTab(pTopTab, "Top + Bottom");

   // Descriptor tab
   QWidget* pDescriptorTab = new QWidget();

   QLabel* pClassReasonLabel = new QLabel("Classification Reason:", pDescriptorTab);
   mpClassReasonCombo = new QComboBox(pDescriptorTab);
   mpClassReasonCombo->setEditable(false);

   QLabel* pDeclassTypeLabel = new QLabel("Declassification Type:", pDescriptorTab);
   mpDeclassTypeCombo = new QComboBox(pDescriptorTab);
   mpDeclassTypeCombo->setEditable(false);

   QLabel* pDeclassDateLabel = new QLabel("Declassification Date:", pDescriptorTab);
   mpDeclassDateEdit = new QDateEdit(pDescriptorTab);
   mpDeclassDateEdit->clear();

   mpResetDeclassDateButton = new QPushButton("Clear", pDescriptorTab);

   QLabel* pFileDowngradeLabel = new QLabel("File Downgrade:", pDescriptorTab);
   mpFileDowngradeCombo = new QComboBox(pDescriptorTab);
   mpFileDowngradeCombo->setEditable(false);

   QLabel* pDowngradeDateLabel = new QLabel("Downgrade Date:", pDescriptorTab);
   mpDowngradeDateEdit = new QDateEdit(pDescriptorTab);

   mpResetDowngradeDateButton = new QPushButton("Clear", pDescriptorTab);

   QLabel* pFileControlLabel = new QLabel("File Control:", pDescriptorTab);
   mpFileControlCombo = new QComboBox(pDescriptorTab);
   mpFileControlCombo->setEditable(false);

   QLabel* pDescriptionLabel = new QLabel("Description:", pDescriptorTab);
   mpDescriptionEdit = new QTextEdit(pDescriptorTab);

   QLabel* pCopyNumberLabel = new QLabel("Copy Number:", pDescriptorTab);
   mpCopyNumberSpinBox = new QSpinBox(pDescriptorTab);
   mpCopyNumberSpinBox->setMinimum(0);

   QLabel* pNumberOfCopiesLabel = new QLabel("Number of Copies:", pDescriptorTab);
   mpNumberOfCopiesSpinBox = new QSpinBox(pDescriptorTab);
   mpNumberOfCopiesSpinBox->setMinimum(0);

   QGridLayout* pDescriptorGrid = new QGridLayout(pDescriptorTab);
   pDescriptorGrid->setMargin(10);
   pDescriptorGrid->setSpacing(5);
   pDescriptorGrid->addWidget(pClassReasonLabel, 0, 0);
   pDescriptorGrid->addWidget(mpClassReasonCombo, 0, 1);
   pDescriptorGrid->addWidget(pDeclassTypeLabel, 1, 0);
   pDescriptorGrid->addWidget(mpDeclassTypeCombo, 1, 1);
   pDescriptorGrid->addWidget(pDeclassDateLabel, 2, 0);
   pDescriptorGrid->addWidget(mpDeclassDateEdit, 2, 1);
   pDescriptorGrid->addWidget(mpResetDeclassDateButton, 2, 2);
   pDescriptorGrid->addWidget(pFileDowngradeLabel, 3, 0);
   pDescriptorGrid->addWidget(mpFileDowngradeCombo, 3, 1);
   pDescriptorGrid->addWidget(pDowngradeDateLabel, 4, 0);
   pDescriptorGrid->addWidget(mpDowngradeDateEdit, 4, 1);
   pDescriptorGrid->addWidget(mpResetDowngradeDateButton, 4, 2);
   pDescriptorGrid->addWidget(pFileControlLabel, 5, 0);
   pDescriptorGrid->addWidget(mpFileControlCombo, 5, 1);
   pDescriptorGrid->addWidget(pDescriptionLabel, 6, 0, Qt::AlignTop);
   pDescriptorGrid->addWidget(mpDescriptionEdit, 6, 1);
   pDescriptorGrid->addWidget(pCopyNumberLabel, 7, 0);
   pDescriptorGrid->addWidget(mpCopyNumberSpinBox, 7, 1, Qt::AlignLeft);
   pDescriptorGrid->addWidget(pNumberOfCopiesLabel, 8, 0);
   pDescriptorGrid->addWidget(mpNumberOfCopiesSpinBox, 8, 1, Qt::AlignLeft);
   pDescriptorGrid->setRowStretch(6, 10);
   pDescriptorGrid->setColumnStretch(1, 10);

   mpTabWidget->addTab(pDescriptorTab, "Descriptor");

   // Buttons
   QPushButton* pAddToFavoritesButton = new QPushButton("Add to favorites", this);
   QPushButton* pRemoveFromFavoritesButton = new QPushButton("Remove from favorites", this);

   QHBoxLayout* pButtonLayout = new QHBoxLayout();
   pButtonLayout->setMargin(0);
   pButtonLayout->setSpacing(5);
   pButtonLayout->addStretch();
   pButtonLayout->addWidget(pAddToFavoritesButton);
   pButtonLayout->addWidget(pRemoveFromFavoritesButton);

   // Layout
   QGridLayout* pGrid = new QGridLayout(this);
   pGrid->setMargin(0);
   pGrid->setSpacing(10);
   pGrid->addWidget(pClassificationLabel, 0, 0, 1, 2);
   pGrid->addWidget(mpClassLevelCombo, 0, 2);
   pGrid->addWidget(mpTabWidget, 1, 0, 1, 3);
   pGrid->addWidget(pFavoritesLabel, 2, 0);
   pGrid->addWidget(mpFavoritesCombo, 2, 1, 1, 2);
   pGrid->addLayout(pButtonLayout, 3, 0, 1, 3);
   pGrid->setRowStretch(1, 10);
   pGrid->setColumnStretch(2, 10);

   // Initialization
   resetToDefault();
   deserializeFavorites();
   updateFavoritesCombo();
   pFieldList->setCurrentRow(0);
   checkAddButton(mpValueStack->currentIndex());

   // Connections
   VERIFYNR(connect(mpClassLevelCombo, SIGNAL(activated(const QString&)), this, SLOT(setLevel(const QString&))));
   VERIFYNR(connect(mpValueStack, SIGNAL(currentChanged(int)), this, SLOT(updateSize())));
   VERIFYNR(connect(mpValueStack, SIGNAL(currentChanged(int)), this, SLOT(checkAddButton(int))));
   VERIFYNR(connect(mpCodewordList, SIGNAL(itemSelectionChanged()), this, SLOT(setCodewords())));
   VERIFYNR(connect(mpSystemList, SIGNAL(itemSelectionChanged()), this, SLOT(setSystem())));
   VERIFYNR(connect(mpFileReleasingList, SIGNAL(itemSelectionChanged()), this, SLOT(setFileReleasing())));
   VERIFYNR(connect(mpCountryCodeList, SIGNAL(itemSelectionChanged()), this, SLOT(setCountryCode())));
   VERIFYNR(connect(mpExemptionList, SIGNAL(itemSelectionChanged()), this, SLOT(setDeclassificationExemption())));
   VERIFYNR(connect(mpAddButton, SIGNAL(clicked()), this, SLOT(addListItem())));
   VERIFYNR(connect(pResetButton, SIGNAL(clicked()), this, SLOT(resetList())));
   VERIFYNR(connect(mpClassReasonCombo, SIGNAL(activated(const QString&)), this,
      SLOT(setClassificationReason(const QString&))));
   VERIFYNR(connect(mpDeclassTypeCombo, SIGNAL(activated(const QString&)), this,
      SLOT(setDeclassificationType(const QString&))));
   VERIFYNR(connect(mpDeclassDateEdit, SIGNAL(dateChanged(const QDate&)), this,
      SLOT(setDeclassificationDate(const QDate&))));
   VERIFYNR(connect(mpResetDeclassDateButton, SIGNAL(clicked()), this, SLOT(resetDeclassificationDate())));
   VERIFYNR(connect(mpFileDowngradeCombo, SIGNAL(activated(const QString&)), this,
      SLOT(setFileDowngrade(const QString&))));
   VERIFYNR(connect(mpDowngradeDateEdit, SIGNAL(dateChanged(const QDate&)), this,
      SLOT(setDowngradeDate(const QDate&))));
   VERIFYNR(connect(mpResetDowngradeDateButton, SIGNAL(clicked()), this, SLOT(resetDowngradeDate())));
   VERIFYNR(connect(mpFileControlCombo, SIGNAL(activated(const QString&)), this,
      SLOT(setFileControl(const QString&))));
   VERIFYNR(connect(mpDescriptionEdit, SIGNAL(textChanged()), this, SLOT(setDescription())));
   VERIFYNR(connect(mpCopyNumberSpinBox, SIGNAL(valueChanged(const QString&)), this,
      SLOT(setFileCopyNumber(const QString&))));
   VERIFYNR(connect(mpNumberOfCopiesSpinBox, SIGNAL(valueChanged(const QString&)), this,
      SLOT(setFileNumberOfCopies(const QString&))));
   VERIFYNR(connect(mpFavoritesCombo, SIGNAL(activated(const QString&)), this, SLOT(favoriteSelected())));
   VERIFYNR(connect(pAddToFavoritesButton, SIGNAL(clicked()), this, SLOT(addFavoriteItem())));
   VERIFYNR(connect(pRemoveFromFavoritesButton, SIGNAL(clicked()), this, SLOT(removeFavoriteItem())));
}

ClassificationWidget::~ClassificationWidget()
{
   if (mpClass.get() != NULL)
   {
      mpClass->detach(SIGNAL_NAME(Subject, Modified), Slot(this, &ClassificationWidget::classificationModified));
   }

   clearFavorites();
}

void ClassificationWidget::setClassification(Classification* pClassification, bool initializeWidgets)
{
   if (pClassification != NULL)
   {
      if (mpClass.get() != NULL)
      {
         mpClass->detach(SIGNAL_NAME(Subject, Modified), Slot(this, &ClassificationWidget::classificationModified));
      }

      mpClass.reset(pClassification);
      if (initializeWidgets == true)
      {
         mNeedsInitialization = true;
         initialize();
      }
      else
      {
         mpClassLevelCombo->setCurrentIndex(-1);
         mpTabWidget->setEnabled(false);
         mpMarkingsEdit->setPlainText(QString());
      }
   }
}

void ClassificationWidget::showEvent(QShowEvent* pEvent)
{
   QWidget::showEvent(pEvent);

   if (mpClass.get() != NULL)
   {
      mpClass->detach(SIGNAL_NAME(Subject, Modified), Slot(this, &ClassificationWidget::classificationModified));
      initialize();
   }
}

void ClassificationWidget::hideEvent(QHideEvent* pEvent)
{
   QWidget::hideEvent(pEvent);

   if (mpClass.get() != NULL)
   {
      mpClass->attach(SIGNAL_NAME(Subject, Modified), Slot(this, &ClassificationWidget::classificationModified));
   }
}

void ClassificationWidget::resetToDefault(QWidget* pWidget)
{
   // Get the default directory from the options
   QString strDefaultDir = QDir::currentPath();

   const Filename* pSupportFilesPath = ConfigurationSettings::getSettingSupportFilesPath();
   if (pSupportFilesPath != NULL)
   {
      strDefaultDir = QString::fromStdString(pSupportFilesPath->getFullPathAndName() + SLASH + "SecurityMarkings");
   }

   if (strDefaultDir.isEmpty() == false)
   {
      strDefaultDir += "/";
   }

   if ((pWidget == NULL) || (pWidget == mpClassLevelCombo))
   {
      populateListFromFile(mpClassLevelCombo, strDefaultDir + "ClassificationLevels.txt", false);
   }

   Service<UtilityServices> pUtilities;

   if ((pWidget == NULL) || (pWidget == mpCodewordList))
   {
      vector<string> codewords = pUtilities->getCodewords();
      mpCodewordList->clear();
      for (vector<string>::iterator iter = codewords.begin(); iter != codewords.end(); ++iter)
      {
         mpCodewordList->addItem(QString::fromStdString((*iter)));
      }
   }
   if ((pWidget == NULL) || (pWidget == mpSystemList))
   {
      vector<string> systems = pUtilities->getSystems();
      mpSystemList->clear();
      for (vector<string>::iterator iter = systems.begin(); iter != systems.end(); ++iter)
      {
         mpSystemList->addItem(QString::fromStdString((*iter)));
      }
   }
   if ((pWidget == NULL) || (pWidget == mpCountryCodeList))
   {
      vector<string> countryCodes = pUtilities->getCountryCodes();
      mpCountryCodeList->clear();
      for (vector<string>::iterator iter = countryCodes.begin(); iter != countryCodes.end(); ++iter)
      {
         mpCountryCodeList->addItem(QString::fromStdString((*iter)));
      }
   }
   if ((pWidget == NULL) || (pWidget == mpFileReleasingList))
   {
      vector<string> fileReleasing = pUtilities->getFileReleasing();
      mpFileReleasingList->clear();
      for (vector<string>::iterator iter = fileReleasing.begin(); iter != fileReleasing.end(); ++iter)
      {
         mpFileReleasingList->addItem(QString::fromStdString((*iter)));
      }
   }
   if ((pWidget == NULL) || (pWidget == mpExemptionList))
   {
      vector<string> exemption = pUtilities->getDeclassificationExemptions();
      mpExemptionList->clear();
      for (vector<string>::iterator iter = exemption.begin(); iter != exemption.end(); ++iter)
      {
         mpExemptionList->addItem(QString::fromStdString((*iter)));
      }
   }
   if ((pWidget == NULL) || (pWidget == mpClassReasonCombo))
   {
      vector<string> classReason = pUtilities->getClassificationReasons();
      mpClassReasonCombo->clear();
      for (vector<string>::iterator iter = classReason.begin(); iter != classReason.end(); ++iter)
      {
         mpClassReasonCombo->addItem(QString::fromStdString((*iter)));
      }
      mpClassReasonCombo->setCurrentIndex(-1);
   }
   if ((pWidget == NULL) || (pWidget == mpDeclassTypeCombo))
   {
      vector<string> declassType = pUtilities->getDeclassificationTypes();
      mpDeclassTypeCombo->clear();
      for (vector<string>::iterator iter = declassType.begin(); iter != declassType.end(); ++iter)
      {
         mpDeclassTypeCombo->addItem(QString::fromStdString((*iter)));
      }
      mpDeclassTypeCombo->setCurrentIndex(-1);
   }
   if ((pWidget == NULL) || (pWidget == mpFileDowngradeCombo))
   {
      vector<string> fileDowngrade = pUtilities->getFileDowngrades();
      mpFileDowngradeCombo->clear();
      for (vector<string>::iterator iter = fileDowngrade.begin(); iter != fileDowngrade.end(); ++iter)
      {
         mpFileDowngradeCombo->addItem(QString::fromStdString((*iter)));
      }
      mpFileDowngradeCombo->setCurrentIndex(-1);
   }
   if ((pWidget == NULL) || (pWidget == mpFileControlCombo))
   {
      vector<string> fileControl = pUtilities->getFileControls();
      mpFileControlCombo->clear();
      for (vector<string>::iterator iter = fileControl.begin(); iter != fileControl.end(); ++iter)
      {
         mpFileControlCombo->addItem(QString::fromStdString((*iter)));
      }
      mpFileControlCombo->setCurrentIndex(-1);
   }
}

void ClassificationWidget::initialize()
{
   if ((mpClass.get() == NULL) || (mNeedsInitialization == false) || (isVisible() == false))
   {
      return;
   }

   mNeedsInitialization = false;

   // Level
   int index = -1;

   string text = mpClass->getLevel();
   if (text.empty() == false)
   {
      QString strText = QString::fromStdString(text);
      for (int i = 0; i < mpClassLevelCombo->count(); ++i)
      {
         QString strItemText = mpClassLevelCombo->itemText(i);
         if ((strItemText == strText) || (strItemText[0] == strText[0]))
         {
            index = i;
            break;
         }
      }
   }

   mpClassLevelCombo->blockSignals(true);
   mpClassLevelCombo->setCurrentIndex(index);
   mpClassLevelCombo->blockSignals(false);

   mpTabWidget->setEnabled(mpClass->getLevel() != "U");

   // Codewords
   text = mpClass->getCodewords();
   selectListFromString(mpCodewordList, QString::fromStdString(text));

   // System
   text = mpClass->getSystem();
   selectListFromString(mpSystemList, QString::fromStdString(text));

   // File releasing
   text = mpClass->getFileReleasing();
   selectListFromString(mpFileReleasingList, QString::fromStdString(text));

   // Country codes
   text = mpClass->getCountryCode();
   selectListFromString(mpCountryCodeList, QString::fromStdString(text));

   // Declassification exemption
   text = mpClass->getDeclassificationExemption();
   selectListFromString(mpExemptionList, QString::fromStdString(text));

   // Classification reason
   text = mpClass->getClassificationReason();
   selectComboFromString(mpClassReasonCombo, QString::fromStdString(text));

   // Declassification type
   text = mpClass->getDeclassificationType();
   selectComboFromString(mpDeclassTypeCombo, QString::fromStdString(text));

   // Declassification date
   const DateTime* pDateTime = mpClass->getDeclassificationDate();
   setDateEdit(mpDeclassDateEdit, pDateTime);

   // File downgrade
   index = -1;

   text = mpClass->getFileDowngrade();
   if (text.empty() == false)
   {
      QString strText = QString::fromStdString(text);
      for (int i = 0; i < mpFileDowngradeCombo->count() && i < mpClassLevelCombo->count(); ++i)
      {
         QString strItemText = mpClassLevelCombo->itemText(i);
         if (strItemText == strText)
         {
            index = i;
            break;
         }
      }
   }

   mpFileDowngradeCombo->blockSignals(true);
   mpFileDowngradeCombo->setCurrentIndex(index);
   mpFileDowngradeCombo->blockSignals(false);

   // Downgrade date
   pDateTime = mpClass->getDowngradeDate();
   setDateEdit(mpDowngradeDateEdit, pDateTime);

   // File control
   text = mpClass->getFileControl();
   selectComboFromString(mpFileControlCombo, QString::fromStdString(text));

   // Description
   mpDescriptionEdit->blockSignals(true);
   mpDescriptionEdit->setPlainText(QString::fromStdString(mpClass->getDescription()));
   mpDescriptionEdit->blockSignals(false);

   // Copy number
   mpCopyNumberSpinBox->blockSignals(true);
   mpCopyNumberSpinBox->setValue((QString::fromStdString(mpClass->getFileCopyNumber())).toInt());
   mpCopyNumberSpinBox->blockSignals(false);

   // File number of copies
   mpNumberOfCopiesSpinBox->blockSignals(true);
   mpNumberOfCopiesSpinBox->setValue((QString::fromStdString(mpClass->getFileNumberOfCopies())).toInt());
   mpNumberOfCopiesSpinBox->blockSignals(false);

   // Update the preview markings
   updateMarkings();
}

void ClassificationWidget::classificationModified(Subject& subject, const string& signal, const boost::any& value)
{
   mNeedsInitialization = true;
}

void ClassificationWidget::setLevel(const QString& level)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   if (level.isEmpty() == false)
   {
      string classLevel = level.toStdString();
      classLevel = classLevel.substr(0, 1);
      mpClass->setLevel(classLevel);
   }
   else if (mpClass->getLevel().empty() == false)
   {
      mpClass->setLevel(string());
   }

   bool enableTabs = (mpClass->getLevel() != "U");
   if (enableTabs == false)
   {
      // Reset the values on the tab widget since the Classification object clears all fields
      mNeedsInitialization = true;
      initialize();
   }
   else
   {
      updateMarkings();
   }

   mpTabWidget->setEnabled(enableTabs);
}

void ClassificationWidget::setCodewords()
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   QString strText = getListString(mpCodewordList);
   if (strText.isEmpty() == false)
   {
      mpClass->setCodewords(strText.toStdString());
   }
   else if (mpClass->getCodewords().empty() == false)
   {
      mpClass->setCodewords(string());
   }

   updateMarkings();
}

void ClassificationWidget::setSystem()
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   QString strText = getListString(mpSystemList);
   if (strText.isEmpty() == false)
   {
      mpClass->setSystem(strText.toStdString());
   }
   else if (mpClass->getSystem().empty() == false)
   {
      mpClass->setSystem(string());
   }

   updateMarkings();
}

void ClassificationWidget::setFileReleasing()
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   QString strText = getListString(mpFileReleasingList);
   if (strText.isEmpty() == false)
   {
      mpClass->setFileReleasing(strText.toStdString());
   }
   else if (mpClass->getFileReleasing().empty() == false)
   {
      mpClass->setFileReleasing(string());
   }

   updateMarkings();
}

void ClassificationWidget::setCountryCode()
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   QString strText = getListString(mpCountryCodeList);
   if (strText.isEmpty() == false)
   {
      mpClass->setCountryCode(strText.toStdString());
   }
   else if (mpClass->getCountryCode().empty() == false)
   {
      mpClass->setCountryCode(string());
   }

   updateMarkings();
}

void ClassificationWidget::setDeclassificationExemption()
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   QString strText = getListString(mpExemptionList);
   if (strText.isEmpty() == false)
   {
      mpClass->setDeclassificationExemption(strText.toStdString());
   }
   else if (mpClass->getDeclassificationExemption().empty() == false)
   {
      mpClass->setDeclassificationExemption(string());
   }

   updateMarkings();
}

void ClassificationWidget::setClassificationReason(const QString& reason)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   if (reason.isEmpty() == false)
   {
      string classReason = reason.toStdString();
      classReason = classReason.substr(0, 1);
      mpClass->setClassificationReason(classReason);
   }
   else if (mpClass->getClassificationReason().empty() == false)
   {
      mpClass->setClassificationReason(string());
   }
}

void ClassificationWidget::setDeclassificationType(const QString& declassType)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   if (declassType.isEmpty() == false)
   {
      string declassTypeText = declassType.toStdString();
      declassTypeText = declassTypeText.substr(0, 2);
      mpClass->setDeclassificationType(declassTypeText);
   }
   else if (mpClass->getDeclassificationType().empty() == false)
   {
      mpClass->setDeclassificationType(string());
   }
}

void ClassificationWidget::setDeclassificationDate(const QDate& declassDate)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   FactoryResource<DateTime> pDateTime;
   if (declassDate.isValid() == true)
   {
      pDateTime->set(declassDate.year(), declassDate.month(), declassDate.day());
   }

   mpClass->setDeclassificationDate(pDateTime.get());
   updateMarkings();
}

void ClassificationWidget::resetDeclassificationDate()
{
   setDateEdit(mpDeclassDateEdit, NULL);
   setDeclassificationDate(QDate());
}

void ClassificationWidget::setFileDowngrade(const QString& fileDowngrade)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   if (fileDowngrade.isEmpty() == false)
   {
      string fileDowngradeText = fileDowngrade.toStdString();
      fileDowngradeText = fileDowngradeText.substr(0, 1);
      mpClass->setFileDowngrade(fileDowngradeText);
   }
   else if (mpClass->getFileDowngrade().empty() == false)
   {
      mpClass->setFileDowngrade(string());
   }
}

void ClassificationWidget::setDowngradeDate(const QDate& downgradeDate)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   FactoryResource<DateTime> pDateTime;
   if (downgradeDate.isValid() == true)
   {
      pDateTime->set(downgradeDate.year(), downgradeDate.month(), downgradeDate.day());
   }

   mpClass->setDowngradeDate(pDateTime.get());
}

void ClassificationWidget::resetDowngradeDate()
{
   setDateEdit(mpDowngradeDateEdit, NULL);
   setDowngradeDate(QDate());
}

void ClassificationWidget::setFileControl(const QString& fileControl)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   if (fileControl.isEmpty() == false)
   {
      mpClass->setFileControl(fileControl.toStdString());
   }
   else if (mpClass->getFileControl().empty() == false)
   {
      mpClass->setFileControl(string());
   }
}

void ClassificationWidget::setDescription()
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   QString strText = mpDescriptionEdit->toPlainText();
   if (strText.isEmpty() == false)
   {
      mpClass->setDescription(strText.toStdString());
   }
   else if (mpClass->getDescription().empty() == false)
   {
      mpClass->setDescription(string());
   }
}

void ClassificationWidget::setFileCopyNumber(const QString& copyNumber)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   if (copyNumber.isEmpty() == false)
   {
      mpClass->setFileCopyNumber(copyNumber.toStdString());
   }
   else if (mpClass->getFileCopyNumber().empty() == false)
   {
      mpClass->setFileCopyNumber(string());
   }
}

void ClassificationWidget::setFileNumberOfCopies(const QString& numberOfCopies)
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   if (numberOfCopies.isEmpty() == false)
   {
      mpClass->setFileNumberOfCopies(numberOfCopies.toStdString());
   }
   else if (mpClass->getFileNumberOfCopies().empty() == false)
   {
      mpClass->setFileNumberOfCopies(string());
   }
}

QString ClassificationWidget::getListString(QListWidget* pListWidget)
{
   if (pListWidget == NULL)
   {
      return QString();
   }

   QString strText;

   QList<QListWidgetItem*> selectedItems = pListWidget->selectedItems();
   for (int j = 0; j < pListWidget->count(); ++j)
   {
      QListWidgetItem* pItem = pListWidget->item(j);
      if (pItem->isSelected())
      {
         if (pItem != NULL)
         {
            if (strText.isEmpty() == false)
            {
               strText += " ";
            }

            QString strItemText = pItem->text();
            if (strItemText.isEmpty() == false)
            {
               strItemText.replace('\\', "\\\\");
               strItemText.replace(' ', "\\ ");
               strText += strItemText;
            }
         }
      }
   }

   return strText;
}

void ClassificationWidget::selectListFromString(QListWidget* pListWidget, const QString& strText)
{
   if (pListWidget == NULL)
   {
      return;
   }

   // There are signals emitted whenever there is a change to the listboxes.
   // We really only want the signals when changed through the GUI.  Simulate
   // by blocking them when changing the listbox programmatically.
   pListWidget->blockSignals(true);

   pListWidget->clearSelection();
   if (strText.isEmpty() == true)
   {
      pListWidget->blockSignals(false);
      return;
   }

   const char* ptr = StringUtilities::escapedToken(strText.toStdString());

   // setSelected() below will signal to ClassificationWidget::updateMarkings(),
   // which indirectly calls selectListFromString.  Block the signal to prevent this.
   while (ptr != NULL)
   {
      QList<QListWidgetItem*> items = pListWidget->findItems(QString::fromLatin1(ptr), Qt::MatchExactly);
      if (items.empty() == false)
      {
         for (int i = 0; i < items.count(); ++i)
         {
            QListWidgetItem* pItem = items[i];
            if (pItem != NULL)
            {
               pListWidget->setItemSelected(pItem, true);
            }
         }
      }
      else
      {
         pListWidget->addItem(QString::fromLatin1(ptr));

         QListWidgetItem* pItem = pListWidget->item(pListWidget->count() - 1);
         if (pItem != NULL)
         {
            pListWidget->setItemSelected(pItem, true);
         }
      }

      ptr = StringUtilities::escapedToken();
   }
   pListWidget->blockSignals(false);
}

void ClassificationWidget::selectComboFromString(QComboBox* pComboBox, const QString& strText)
{
   if (pComboBox == NULL)
   {
      return;
   }

   int index = -1;
   if (strText.isEmpty() == false)
   {
      for (int i = 0; i < pComboBox->count(); ++i)
      {
         if (pComboBox->itemText(i) == strText)
         {
            index = i;
            break;
         }
      }
   }

   pComboBox->blockSignals(true);
   pComboBox->setCurrentIndex(index);
   pComboBox->blockSignals(false);
}

void ClassificationWidget::setDateEdit(QDateEdit* pDateEdit, const DateTime* pDateTime)
{
   if (pDateEdit == NULL)
   {
      return;
   }

   pDateEdit->blockSignals(true);

   if (pDateTime == NULL || pDateTime->isValid() == false)
   {
      // Since QDateEdit cannot display an invalid date, set the date to the default constructed value for the widget
      pDateEdit->setDate(QDate(2000, 1, 1));
   }
   else
   {
#if defined(WIN_API)
      string format = "%#m %#d %#Y %#H %#M %#S";
#else
      string format = "%m %d %Y %H %M %S";
#endif
      string dateString = pDateTime->getFormattedUtc(format);

      int iMonth = 0;
      int iDay = 0;
      int iYear = 0;
      int iHour = 0;
      int iMinute = 0;
      int iSecond = 0;
      sscanf(dateString.c_str(), "%d %d %d %d %d %d", &iMonth, &iDay, &iYear, &iHour, &iMinute, &iSecond);

      QDate dateValue(iYear, iMonth, iDay);
      pDateEdit->setDate(dateValue);
   }

   pDateEdit->blockSignals(false);
}

void ClassificationWidget::addListItem()
{
   QListWidget* pList = static_cast<QListWidget*>(mpValueStack->currentWidget());
   if (pList != NULL)
   {
      QString strItem = QInputDialog::getText(this, "Security Markings", "New Item:");
      if (strItem.isEmpty() == false)
      {
         pList->addItem(strItem);
      }
   }
}

void ClassificationWidget::resetList()
{
   QListWidget* pList = static_cast<QListWidget*>(mpValueStack->currentWidget());
   if (pList != NULL)
   {
      resetToDefault(pList);
      updateMarkings();
   }
}

void ClassificationWidget::updateMarkings()
{
   QString strMarkings;
   if (mpClass.get() != NULL)
   {
      string classificationText;
      mpClass->getClassificationText(classificationText);
      if (classificationText.empty() == false)
      {
         strMarkings = QString::fromStdString(classificationText);
      }
   }

   mpMarkingsEdit->setPlainText(strMarkings);
}

void ClassificationWidget::updateSize()
{
   mpValueStack->updateGeometry();
   mpTabWidget->updateGeometry();
}

void ClassificationWidget::addFavoriteItem()
{
   FactoryResource<Classification> pFactoryClass;
   Classification* pClass = pFactoryClass.release();
   pClass->setClassification(mpClass.get());

   mlstFavorites.push_back(pClass);
   serializeFavorites();
   updateFavoritesCombo();
}

void ClassificationWidget::removeFavoriteItem()
{
   int iRemoveClass = mpFavoritesCombo->currentIndex();
   if (iRemoveClass >= 0 && (unsigned int)iRemoveClass < mlstFavorites.size())
   {
      vector<Classification*>::iterator itr = mlstFavorites.begin()+iRemoveClass;

      // vector owns the classification, need to delete it
      Classification* pClassification = (*itr);
      if (pClassification != NULL)
      {
         Service<ApplicationServices> pApp;
         if (pApp.get() != NULL)
         {
            ObjectFactory* pObjFact = pApp->getObjectFactory();
            if (pObjFact != NULL)
            {
               pObjFact->destroyObject(pClassification, "Classification");
            }
         }
      }

      mlstFavorites.erase(itr, itr+1);
      serializeFavorites();
      updateFavoritesCombo();
   }
}

void ClassificationWidget::favoriteSelected()
{
   if (mpClass.get() == NULL)
   {
      return;
   }

   int index = mpFavoritesCombo->currentIndex();
   if (index < 0)
   {
      return;
   }

   Classification* pClass = mlstFavorites[index];
   if (pClass != NULL)
   {
      mpClass->setClassification(pClass);
      mNeedsInitialization = true;
      initialize();
   }
}

bool ClassificationWidget::serializeFavorites()
{
   FactoryResource<DynamicObject> pNewFavorites;
   VERIFY(pNewFavorites.get() != NULL);

   for (unsigned int i = 0; i < mlstFavorites.size(); ++i)
   {
      string level = mlstFavorites[i]->getLevel();
      string codewords = mlstFavorites[i]->getCodewords();
      string system = mlstFavorites[i]->getSystem();
      string releasability = mlstFavorites[i]->getFileReleasing();
      string countries = mlstFavorites[i]->getCountryCode();
      string exemption = mlstFavorites[i]->getDeclassificationExemption();
      const DateTime* pDeclassificationDate = mlstFavorites[i]->getDeclassificationDate();

      string attributeName = string("favorite") + StringUtilities::toDisplayString<unsigned int>(i);

      FactoryResource<DynamicObject> pClassificationObject;
      pClassificationObject->setAttribute("level", level);
      pClassificationObject->setAttribute("codewords", codewords);
      pClassificationObject->setAttribute("system", system);
      pClassificationObject->setAttribute("releasability", releasability);
      pClassificationObject->setAttribute("countries", countries);
      pClassificationObject->setAttribute("exemption", exemption);
      if (pDeclassificationDate != NULL && pDeclassificationDate->isValid())
      {
         pClassificationObject->setAttribute("declassificationDate", *pDeclassificationDate);
      }

      // add attribute with the classification text to use for validation when favorites are deserialized
      string classificationText;
      mlstFavorites[i]->getClassificationText(classificationText);
      pClassificationObject->setAttribute("classificationText", classificationText);

      pNewFavorites->setAttribute(attributeName, *pClassificationObject.get());
   }
   ClassificationWidget::setSettingFavorites(pNewFavorites.get());
   return true;
}

bool ClassificationWidget::deserializeFavorites()
{
   const DynamicObject* pFavorites = ClassificationWidget::getSettingFavorites();
   if (pFavorites == NULL)
   {
      return false;
   }
   
   clearFavorites();

   vector<string> previews;
   pFavorites->getAttributeNames(previews);
   for (unsigned int i = 0; i < previews.size(); ++i)
   {
      const DataVariant& var = pFavorites->getAttribute(previews[i]);
      const DynamicObject* pFavDynObj = dv_cast<DynamicObject>(&var);
      if (pFavDynObj == NULL)
      {
         continue;
      }
      const string* pLevel = dv_cast<string>(&pFavDynObj->getAttribute("level"));
      const string* pCodewords = dv_cast<string>(&pFavDynObj->getAttribute("codewords"));
      const string* pSystem = dv_cast<string>(&pFavDynObj->getAttribute("system"));
      const string* pReleasability = dv_cast<string>(&pFavDynObj->getAttribute("releasability"));
      const string* pCountries = dv_cast<string>(&pFavDynObj->getAttribute("countries"));
      const string* pExemption = dv_cast<string>(&pFavDynObj->getAttribute("exemption"));
      const DateTime* pDeclassDate = dv_cast<DateTime>(&pFavDynObj->getAttribute("declassificationDate"));
      const string* pClassificationText = dv_cast<string>(&pFavDynObj->getAttribute("classificationText"));

      FactoryResource<Classification> pClass;
      if (pLevel != NULL)
      {
         pClass->setLevel(*pLevel);
      }
      if (pCodewords != NULL)
      {
         pClass->setCodewords(*pCodewords);
      }
      if (pSystem != NULL)
      {
         pClass->setSystem(*pSystem);
      }
      if (pReleasability != NULL)
      {
         pClass->setFileReleasing(*pReleasability);
      }
      if (pCountries != NULL)
      {
         pClass->setCountryCode(*pCountries);
      }
      if (pExemption != NULL)
      {
         pClass->setDeclassificationExemption(*pExemption);
      }
      if (pDeclassDate != NULL && pDeclassDate->isValid())
      {
         pClass->setDeclassificationDate(pDeclassDate);
      }

      // sanity check - make sure our new classification matches the old favorite
      string classText;
      pClass->getClassificationText(classText);
      if (pClassificationText == NULL || classText != *pClassificationText)
      {
         // if not, kill the old setting, do not add our own
         const DynamicObject* pOriginalFavorites = ClassificationWidget::getSettingFavorites();
         if (pOriginalFavorites != NULL)
         {
            FactoryResource<DynamicObject> pNewFavorites;
            pNewFavorites->merge(pOriginalFavorites);
            pNewFavorites->removeAttribute(previews[i]);
            ClassificationWidget::setSettingFavorites(pNewFavorites.get());
         }
      }
      else
      {
         mlstFavorites.push_back(pClass.release());
      }
   }

   return true;
}

void ClassificationWidget::updateFavoritesCombo()
{
   mpFavoritesCombo->clear();
   for (unsigned int i = 0; i < mlstFavorites.size(); ++i)
   {
      string strClassText;
      mlstFavorites[i]->getClassificationText(strClassText);
      mpFavoritesCombo->insertItem(i, QString::fromStdString(strClassText));
   }
}

void ClassificationWidget::clearFavorites()
{
   Service<ApplicationServices> pApp;
   if (pApp.get() != NULL)
   {
      ObjectFactory* pObjFact = pApp->getObjectFactory();
      if (pObjFact != NULL)
      {
         for (unsigned int i = 0; i < mlstFavorites.size(); ++i)
         {
            pObjFact->destroyObject(mlstFavorites[i], "Classification");
         }
      }
   }

   mlstFavorites.clear();
}

void ClassificationWidget::checkAddButton(int iIndex)
{
   QWidget* pWidget = mpValueStack->widget(iIndex);
   if (pWidget == mpCodewordList || pWidget == mpFileReleasingList || pWidget == mpCountryCodeList)
   {
      mpAddButton->setDisabled(true);
   }
   else
   {
      mpAddButton->setDisabled(false);
   }
}
