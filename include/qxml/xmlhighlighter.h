#pragma once

#include <QSyntaxHighlighter>
#include <QTextDocument>

class XmlEventParser;

class XmlHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT
  enum BlockState
  {
    NodeComplete = 0,
    NodeIncomplete,
  };
  struct FormatSize
  {
    int start = -1;
    int length = 0;
  };

public:

  explicit XmlHighlighter(XmlEventParser* parser,
                          QTextDocument* parent = nullptr);

  //! Returns the text colour.
  QColor textColor();

  //! Sets the text colour for all non current line blocks.
  void setTextColor(const QColor& color);

  //! Returns the background colour.
  QColor background();
  //! Sets the background colour for all non current line blocks.
  void setBackground(const QColor& color);

  //! Gets the colour for bracket tag/matching
  QColor matchColor();
  //! Sets the foreground and, optionally, the background colour for
  //! tag start (<) / tag end (>) and bracket matching
  void setMatchColor(const QColor& color);

  //! Gets the colour for bracket tag/matching
  QColor nameColor();
  //! Sets the foreground colour for tag names
  void setNameColor(const QColor& color);

  //! Gets the foreground colour for attribute names
  QColor attrColor();
  //! Sets the foreground colour for attribute names
  void setAttrColor(const QColor& color);

  //! Gets the foreground colour for attribute values
  QColor valueColor();
  //! Sets the foreground colour for attribute values
  void setValueColor(const QColor& color);

  //! Gets the foreground colour for values within single quotes
  QColor sQuoteColor();
  //! Sets the foreground colour for values within single quotes
  void setSQuoteColor(const QColor& color);

  //! Gets the foreground colour for values within double quotes
  QColor dQuoteColor();
  //! Sets the foreground colour for values within double quotes
  void setDQuoteColor(const QColor& color);

  //! Gets the foreground colour for comments
  QColor commentColor();
  //! Sets the foreground colour for comments
  void setCommentColor(const QColor& color);

  //! Gets the foreground colour for cdata
  const QColor& cdataColor() const;
  //! Sets the foreground colour for cdata
  void setCdataColor(const QColor& color);

  //! Gets the foreground colour for processing instructions
  const QColor& piTargetColor() const;
  //! Sets the foreground colour for processing instructions
  void setPiTargetColor(const QColor& color);

  const QColor& piDataColor() const;
  void setPiDataColor(const QColor& PiDataColor);

  QColor xmlolor() const;
  void setXmlolor(const QColor& Xmlolor);

protected:
  //! \reimplements{QSyntaxHighlighter::highlightBlock}
  void highlightBlock(const QString& text);

private:
  XmlEventParser* m_parser;

  QColor m_xmlColor;
  QColor m_textColor;
  QColor m_backgroundColor;
  QColor m_matchColor;
  QColor m_matchBackgroundColor;
  QColor m_nameColor;
  QColor m_attrColor;
  QColor m_valueColor;
  QColor m_sQuoteColor;
  QColor m_dQuoteColor;
  QColor m_commentColor;
  QColor m_errorColor;
  QColor m_cdataColor;
  QColor m_piTargetColor;
  QColor m_piDataColor;

  QTextCharFormat m_xmlFormat;
  QTextCharFormat m_textFormat;
  QTextCharFormat m_matchFormat;
  QTextCharFormat m_nameFormat;
  QTextCharFormat m_attrFormat;
  QTextCharFormat m_valueFormat;
  QTextCharFormat m_sQuoteFormat;
  QTextCharFormat m_dQuoteFormat;
  QTextCharFormat m_commentFormat;
  QTextCharFormat m_errorFormat;
  QTextCharFormat m_cdataFormat;
  QTextCharFormat m_piTargetFormat;
  QTextCharFormat m_piDataFormat;

  bool isFormatable(int start, int length, int blockStart, int textLength, FormatSize &result);
};
