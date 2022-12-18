#include "qxml/xmledit.h"
#include "qxml/xmleventparser.h"
#include "qxml/xmlhighlighter.h"
//#include "widgets/settingsdialog.h"

#include <JlCompress.h>

//====================================================================
//=== XmlEdit
//====================================================================
XmlEdit::XmlEdit(QWidget* parent)
  : LNPlainTextEdit(this)
  , m_parser(new XmlEventParser(LNPlainTextEdit::document(), this))
  , m_highlighter(new XmlHighlighter(m_parser, LNPlainTextEdit::document()))
  , m_parent(parent)
{
  initSettings(new XmlEditSettings(m_highlighter, parent));
}

XmlEdit::XmlEdit(BaseConfig* config, QWidget* parent)
  : LNPlainTextEdit(config, parent)
  , m_parser(new XmlEventParser(LNPlainTextEdit::document(), this))
  , m_highlighter(new XmlHighlighter(m_parser, LNPlainTextEdit::document()))
  , m_parent(parent)
{
  initSettings(new XmlEditSettings(m_highlighter, parent));
}

void
XmlEdit::initialise()
{
  setAcceptDrops(true);

  m_keyMap->addAction(PreviousBookmark,
                      tr("Previous Bookmark"),
                      Qt::Key_Comma,
                      Qt::ControlModifier);
  m_keyMap->addAction(
    NextBookmark, tr("Next Bookmark"), Qt::Key_Stop, Qt::ControlModifier);
  m_keyMap->addAction(
    AddBookmark, tr("Add Bookmark"), Qt::Key_M, Qt::ControlModifier);
  m_keyMap->addAction(ToggleBookmark,
                      tr("Toggle Bookmark"),
                      Qt::Key_M,
                      Qt::ControlModifier | Qt::AltModifier);
  m_keyMap->addAction(EditBookmark,
                      tr("Edit Bookmark"),
                      Qt::Key_M,
                      Qt::ControlModifier | Qt::ShiftModifier);
  m_keyMap->addAction(ClearBookmarks,
                      tr("Clear Bookmarks"),
                      Qt::Key_C,
                      Qt::ControlModifier | Qt::AltModifier);

  m_keyMap->addAction(Search, tr("Find"), Qt::Key_F, Qt::ControlModifier);
  m_keyMap->addAction(
    SearchNext, tr("Find Next"), Qt::Key_G, Qt::ControlModifier);
  m_keyMap->addAction(SearchPrevious,
                      tr("Find Previous"),
                      Qt::Key_G,
                      Qt::ControlModifier | Qt::ShiftModifier);

  m_keyMap->addAction(
    Preferences, tr("Preferences"), Qt::Key_Comma, Qt::ControlModifier);

  connect(m_parser, &XmlEventParser::sendWarning, this, &XmlEdit::sendWarning);
  connect(m_parser, &XmlEventParser::sendError, this, &XmlEdit::sendError);
}

bool
XmlEdit::isModified() const
{
  return m_modified;
}

const QString
XmlEdit::filename() const
{
  return m_filename;
}

void
XmlEdit::loadFile(const QString& filename)
{
  m_filename = filename;
  QFile file(m_filename);
  if (file.open(QIODevice::ReadOnly)) {
    auto text = file.readAll();
    setText(text);
  }
}

void
XmlEdit::loadFromZip(const QString& zipFile, const QString& href)
{
  m_filename = href;
  m_zipFile = zipFile;
  auto fileName = JlCompress::extractFile(zipFile, href);
  QFile file(fileName);
  if (file.open(QIODevice::ReadOnly)) {
    auto text = file.readAll();
    setText(text);
  }
}

void
XmlEdit::setText(const QString& text)
{
  disconnect(LNPlainTextEdit::document(),
             &QTextDocument::contentsChange,
             this,
             &XmlEdit::textHasChanged);
  QPlainTextEdit::setPlainText(text);
  m_parser->parseString(text);
  connect(LNPlainTextEdit::document(),
          &QTextDocument::contentsChange,
          this,
          &XmlEdit::textHasChanged);
  m_highlighter->rehighlight();
}

Node*
XmlEdit::nodeAtPosition(QPoint position)
{
  auto cursor = cursorForPosition(position);
  return m_parser->nodeForPosition(cursor.position());
}

Node*
XmlEdit::nodeAtPosition(int position)
{
  return m_parser->nodeForPosition(position);
}

void
XmlEdit::paintEvent(QPaintEvent* e)
{
  LNPlainTextEdit::paintEvent(e);
}

void
XmlEdit::contextMenuEvent(QContextMenuEvent* event)
{
  auto menu = createStandardContextMenu();
  LNPlainTextEdit::modifyContextMenu(menu);

  auto action = m_keyMap->action(KeyEvent::Preferences);
  if (action) {
    menu->addSeparator();
    connect(action, &QAction::triggered, this, &XmlEdit::optionsDialog);
    menu->addAction(action);
  }

  menu->exec(event->globalPos());
  menu->deleteLater();
}

void
XmlEdit::optionsDialog()
{
  //  auto m_settingsDlg = new SettingsDialog(this);
  //  m_settingsDlg->addTab(this, "Xml Editor");

  //  if (m_settingsDlg->exec() == QDialog::Accepted) {
  //    for (auto i = 0; i < m_settingsDlg->count(); i++) {
  //      auto page = m_settingsDlg->widget(i);
  //      setSettingsPage(page);
  //    }
  //  }
}

// void
// XmlEdit::savePreferences()
//{
//   auto s = qobject_cast<XmlEditSettings*>(m_settings);
//   s->createNode();
// }

// void
// XmlEdit::loadPreferences()
//{
// }

// QList<SettingsWidget*>
// XmlEdit::settingsPages()
//{
//   auto widgets = LNPlainTextEdit::settingsPages();
//   // TODO add your widgets.
//   //  widgets.append(widget);
//   return widgets;
// }

// void
// XmlEdit::setSettingsPages(QList<SettingsWidget*> widgets)
//{
//   if (widgets.isEmpty()) {
//     auto settingsWidget = new XmlEditSettingsWidget(
//       qobject_cast<XmlEditSettings*>(m_settings), m_highlighter, this);
//     if (settingsWidget) {
//       //      LNPlainTextEdit::setSettingsPage(settingsWidget);
//       // TODO HtmlEdit settings.
//     }
//   } else {
//     LNPlainTextEdit::setSettingsPages(widgets);
//   }
// }

void
XmlEdit::textHasChanged(int position, int charsRemoved, int charsAdded)
{
  // TODO
}

//====================================================================
//=== XmlEditSettingsWidget
//====================================================================
bool
XmlEditSettingsWidget::save()
{
  //  LNPlainTextEditSettings::save();
  return true;
}

bool
XmlEditSettingsWidget::load()
{
  // TODO
  return true;
}

//====================================================================
//=== XmlEditSettings
//====================================================================
XmlEditSettings::XmlEditSettings(XmlHighlighter* highlighter, QObject* parent)
  : LNPlainTextEditSettings(parent)
  , m_highlighter(highlighter)
{
}

XmlEditSettings::XmlEditSettings(XmlHighlighter* highlighter,
                                 BaseConfig* config,
                                 QObject* parent)
  : LNPlainTextEditSettings(config, parent)
  , m_highlighter(highlighter)
{
}

YAML::Node
XmlEditSettings::createNode(YAML::Node root, YAML::Node parent)
{
  LNPlainTextEditSettings::createNode(root, parent);

  if (!parent.IsNull()) {
    parent["filename"] = m_filename;
  }

  YAML::Node hNode;

  if (!root["Xml_Highlighter_Colors"]) {
    YAML::Node node;
    node["Xml_Highlighter_Colors"];
    root = node;
    hNode = node;
  } else {
    hNode = root["Xml_Highlighter_Colors"];
  }

  if (!hNode["xml_decl_color"]) {
    YAML::Node node;
    node["xml_decl_color"] = m_highlighter->xmlolor();
    hNode.push_back(node);
  } else {
    hNode["xml_decl_color"] = m_highlighter->xmlolor();
  }

  if (!hNode["text_color"]) {
    YAML::Node node;
    node["text_color"] = m_highlighter->textColor();
    hNode.push_back(node);
  } else {
    hNode["text_color"] = m_highlighter->textColor();
  }

  if (!hNode["attribute_color"]) {
    YAML::Node node;
    node["attribute_color"] = m_highlighter->attrColor();
    hNode.push_back(node);
  } else {
    hNode["attribute_color"] = m_highlighter->attrColor();
  }

  if (!hNode["background"]) {
    YAML::Node node;
    node["background"] = m_highlighter->background();
    hNode.push_back(node);
  } else {
    hNode["background"] = m_highlighter->background();
  }

  if (!hNode["value_color"]) {
    YAML::Node node;
    node["value_color"] = m_highlighter->valueColor();
    hNode.push_back(node);
  } else {
    hNode["value_color"] = m_highlighter->valueColor();
  }

  if (!hNode["name_color"]) {
    YAML::Node node;
    node["name_color"] = m_highlighter->nameColor();
    hNode.push_back(node);
  } else {
    hNode["name_color"] = m_highlighter->nameColor();
  }

  if (!hNode["single_quote_color"]) {
    YAML::Node node;
    node["single_quote_color"] = m_highlighter->sQuoteColor();
    hNode.push_back(node);
  } else {
    hNode["single_quote_color"] = m_highlighter->sQuoteColor();
  }

  if (!hNode["double_quote_color"]) {
    YAML::Node node;
    node["double_quote_color"] = m_highlighter->dQuoteColor();
    hNode.push_back(node);
  } else {
    hNode["double_quote_color"] = m_highlighter->dQuoteColor();
  }

  if (!hNode["comment_color"]) {
    YAML::Node node;
    node["comment_color"] = m_highlighter->commentColor();
    hNode.push_back(node);
  } else {
    hNode["comment_color"] = m_highlighter->commentColor();
  }

  if (!hNode["cdata_color"]) {
    YAML::Node node;
    node["cdata_color"] = m_highlighter->cdataColor();
    hNode.push_back(node);
  } else {
    hNode["cdata_color"] = m_highlighter->cdataColor();
  }

  if (!hNode["proc_inst_target_color"]) {
    YAML::Node node;
    node["proc_inst_target_color"] = m_highlighter->piTargetColor();
    hNode.push_back(node);
  } else {
    hNode["proc_inst_target_color"] = m_highlighter->piTargetColor();
  }

  if (!hNode["proc_inst_data_color"]) {
    YAML::Node node;
    node["proc_inst_data_color"] = m_highlighter->piDataColor();
    hNode.push_back(node);
  } else {
    hNode["proc_inst_data_color"] = m_highlighter->piDataColor();
  }

  return hNode;
}

bool
XmlEditSettings::load()
{
  LNPlainTextEditSettings::load();
  return true;
}

QString
XmlEditSettings::filename() const
{
  return m_filename;
}

void
XmlEditSettings::setFilename(const QString& Filename)
{
  m_filename = Filename;
}
