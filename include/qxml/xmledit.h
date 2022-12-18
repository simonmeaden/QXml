#pragma once

#include "SMLibraries/widgets/lnplaintextedit.h"

#include <QTableWidget>

class XmlEventParser;
class XmlHighlighter;
class Node;
class XmlEdit;

class XmlEditSettings : public LNPlainTextEditSettings
{
  Q_OBJECT
public:
  XmlEditSettings(XmlHighlighter* highlighter, QObject* parent);
  XmlEditSettings(XmlHighlighter *highlighter, BaseConfig* config, QObject* parent);

  YAML::Node createNode(YAML::Node root, YAML::Node parent) override;
  bool load() override;

  QString filename() const;
  void setFilename(const QString& Filename);

private:
  XmlHighlighter* m_highlighter;
  QString m_filename;
};

class XmlEditSettingsWidget : public LNPlainTextEditSettingsWidget
{
  Q_OBJECT

  enum Colors
  {
    NoType,
    Text,
    Background,
    Selection,
    SelectionBackground,
    Parenthesis,
    CurrentLineNumberAreaText,
    CurrentLineNumberAreaBackground,
    LineNumberAreaText,
    LineNumberAreaBackground,
    QuotedString,
    ApostrophiedString,
    TagName,
    AttributeName,
    AttributeValue,
    Comment,
    SpecialChars,
    SpecialCharsBackground,
  };

public:
  XmlEditSettingsWidget(XmlEditSettings* settings,
                        XmlHighlighter* highlighter,
                        XmlEdit* parent);

  // LNPlainTextEdit interface
  bool isModified() const { return false; }

  bool save();
  bool load();

protected:
  void initGui(int& row);

private:
  bool m_modified = false;
  XmlEdit* m_editor;
  XmlHighlighter* m_highlighter;
  XmlEdit* m_display = nullptr;
  XmlEditSettings* m_settings;
  QMap<Colors, QColor> m_colorMap;
  QMap<Colors, QList<QTableWidgetItem*>> m_itemMap;

  void colorChanged(QTableWidgetItem* item);
  void textChanged();
  void backChanged();
  void currLNAreaBackChanged();
  void currLNAreaTextChanged();
  void lnAreaTextChanged();
  void lnAreaBackChanged();
  void specTextChanged();
  void specBackChanged();
  void resetDisplaySize(int size);
};
// Q_DECLARE_METATYPE(HtmlEditSettings::Colors);

class XmlEdit : public LNPlainTextEdit
{
  Q_OBJECT
public:
  XmlEdit(QWidget* parent = nullptr);
  XmlEdit(BaseConfig* config, QWidget* parent = nullptr);

  // LNPlainTextEdit interface
  bool isModified() const override;

  //! Returns the file name loaded via loadFile(const QString&) or
  //! loadHref(const QString&, const QString&)
  const QString filename() const;
  //! Loads the file in href into the editor.
  void loadFile(const QString& filename);
  //! Loads the file in href from the zipped file zipfile.
  void loadFromZip(const QString& zipFile, const QString& href);
  //! Loads plain text into the editor
  void setText(const QString& text);

  //! Returns a pointer to the the Node at the mouse position or nullptr if
  //! no node exists at that point.
  Node* nodeAtPosition(QPoint position);
  //! Returns a pointer to the the Node at the cursor position or nullptr if
  //! no node exists at that position.
  Node* nodeAtPosition(int position);

  void optionsDialog();

//   void savePreferences() override;
//   void loadPreferences() override;

signals:
  void sendError(const QString&);
  void sendWarning(const QString&);

protected:
  //! \reimplements{lNPlainTextEdit::paintEvent(QPaintEvent*)
  void paintEvent(QPaintEvent* e) override;
  //! \reimplements{lNPlainTextEdit::contextMenuEvent(QContextMenuEvent*)
  void contextMenuEvent(QContextMenuEvent* event) override;

private:
  //  QTextDocument* m_document = nullptr;
  XmlEventParser* m_parser;
  XmlHighlighter* m_highlighter;
  QWidget* m_parent;
  bool m_modified;
  QString m_filename;
  QString m_zipFile;

  void textHasChanged(int position, int charsRemoved, int charsAdded);
  void initialise();
};
