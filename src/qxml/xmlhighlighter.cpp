#include "qxml/xmlhighlighter.h"
#include "SMLibraries/utilities/x11colors.h"
#include "qxml/xmleventparser.h"

XmlHighlighter::XmlHighlighter(XmlEventParser* parser, QTextDocument* parent)
  : QSyntaxHighlighter{ parent }
  , m_parser(parser)
  , m_xmlColor(QColorConstants::Svg::cadetblue)
  , m_textColor(QColorConstants::Black)
  , m_backgroundColor(QColorConstants::White)
  , m_matchColor(QColorConstants::X11::chartreuse)
  , m_matchBackgroundColor(QColorConstants::X11::grey50)
  , m_nameColor(QColorConstants::X11::mediumblue)
  , m_attrColor(QColorConstants::Svg::olive)
  , m_valueColor(QColorConstants::X11::midnightblue)
  , m_sQuoteColor(QColorConstants::Svg::darkgreen)
  , m_dQuoteColor(QColorConstants::X11::darkgreen)
  , m_commentColor(QColorConstants::X11::ForestGreen)
  , m_errorColor(QColorConstants::X11::orangered)
  , m_cdataColor(QColorConstants::X11::pink)
  , m_piTargetColor(QColorConstants::Svg::cadetblue)
  , m_piDataColor(QColorConstants::DarkBlue)
{
  m_xmlFormat.setForeground(m_xmlColor);
  m_textFormat.setForeground(m_textColor);
  m_textFormat.setBackground(m_backgroundColor);
  m_matchFormat.setForeground(m_matchColor);
  m_matchFormat.setBackground(m_matchBackgroundColor);
  m_nameFormat.setForeground(m_nameColor);
  m_nameFormat.setBackground(m_backgroundColor);
  m_attrFormat.setForeground(m_attrColor);
  m_attrFormat.setBackground(m_backgroundColor);
  m_valueFormat.setForeground(m_valueColor);
  m_valueFormat.setBackground(m_backgroundColor);
  m_sQuoteFormat.setForeground(m_sQuoteColor);
  m_sQuoteFormat.setBackground(m_backgroundColor);
  m_dQuoteFormat.setForeground(m_dQuoteColor);
  m_dQuoteFormat.setBackground(m_backgroundColor);
  m_cdataFormat.setForeground(m_cdataColor);
  m_cdataFormat.setBackground(m_backgroundColor);
  m_commentFormat.setForeground(m_commentColor);
  m_commentFormat.setBackground(m_backgroundColor);
  m_piTargetFormat.setForeground(m_piTargetColor);
  m_piTargetFormat.setBackground(m_backgroundColor);
  m_piDataFormat.setForeground(m_piDataColor);
  m_piDataFormat.setBackground(m_backgroundColor);
  m_errorFormat.setForeground(m_errorColor);

  setCurrentBlockState(NodeComplete);
}

bool
XmlHighlighter::isFormatable(int start,
                             int length,
                             int blockStart,
                             int textLength,
                             FormatSize& result)
{
  //  auto offsetstart = start + blockStart;
  auto end = start + length;
  auto textend = blockStart + textLength;

  if (end < blockStart || start > textend)
    return false;

  if (start < 0)
    result.start = blockStart;
  else{
    result.start = start - blockStart;
    result.start = (result.start < 0 ? 0 : result.start);
  }

  if (end > textend)
    result.length = start + length - textLength;
  else
    result.length = length;

  return true;
}

void
XmlHighlighter::highlightBlock(const QString& text)
{
  if (m_parser->nodes().isEmpty())
    return;

  auto block = currentBlock();
  auto blockStart = block.position();
  auto textLength = text.length();

  for (auto node : m_parser->nodes()) {
    auto nodeStart = node->start();
    auto nodeEnd = node->end();

    // node is completely outside block
    if (nodeEnd < blockStart || nodeStart >= blockStart + textLength)
      continue;

    // adjust start and end within text.
    nodeStart -= blockStart;
    nodeEnd -= blockStart;
    nodeStart = nodeStart < 0 ? 0 : nodeStart;
    nodeEnd = nodeEnd > textLength ? textLength : nodeEnd;

    FormatSize formatable;

    switch (node->type) {
      case Node::XmlDeclaration: {
        auto n = dynamic_cast<XmlDeclarationNode*>(node);
        if (n) {
          if (isFormatable(n->nameStart() - blockStart,
                           4,
                           blockStart,
                           textLength,
                           formatable)) {
            setFormat(formatable.start, formatable.length, m_xmlFormat);
          }
          if (n->hasVersion()) {
            if (isFormatable(
                  n->versionStart(), 7, blockStart, textLength, formatable)) {
              setFormat(formatable.start, formatable.length, m_attrFormat);
            }
            if (isFormatable(n->versionValueStart(),
                             n->version.length(),
                             blockStart,
                             textLength,
                             formatable)) {
              setFormat(formatable.start, formatable.length, m_valueFormat);
            }
          }
          if (n->hasEncoding()) {
            if (isFormatable(
                  n->encodingStart(), 8, blockStart, textLength, formatable)) {
              setFormat(formatable.start, formatable.length, m_attrFormat);
            }
            if (isFormatable(n->encodingValueStart(),
                             n->encoding.length(),
                             blockStart,
                             textLength,
                             formatable)) {
              setFormat(formatable.start, formatable.length, m_valueFormat);
            }
          }

          if (n->hasStandalone()) {
            if (isFormatable(n->standaloneStart(),
                             10,
                             blockStart,
                             textLength,
                             formatable)) {
              setFormat(formatable.start, formatable.length, m_attrFormat);
            }
            if (isFormatable(n->standaloneValueStart(),
                             n->standalone.length(),
                             blockStart,
                             textLength,
                             formatable)) {
              setFormat(formatable.start, formatable.length, m_valueFormat);
            }
          }
        }
        break;
      }
      case Node::Text: {
        auto n = dynamic_cast<TextNode*>(node);
        if (n) {
          if (isFormatable(
                n->start(), n->length(), blockStart, textLength, formatable)) {
            setFormat(formatable.start, formatable.length, m_textFormat);
          }
        }
        break;
      }
      case Node::Start: {
        auto n = dynamic_cast<StartNode*>(node);
        if (n) {
          if (isFormatable(
                n->start(), n->length(), blockStart, textLength, formatable)) {
            setFormat(formatable.start, formatable.length, m_textFormat);
          }
          if (isFormatable(n->nameStart(),
                           n->nameLength(),
                           blockStart,
                           textLength,
                           formatable)) {
            setFormat(formatable.start, formatable.length, m_nameFormat);
          }
          for (auto& a : n->attributes) {
            if (isFormatable(a->nameStart(),
                             a->nameLength(),
                             blockStart,
                             textLength,
                             formatable)) {
              setFormat(formatable.start, formatable.length, m_attrFormat);
            }
            if (a->hasValue()) {
              if (isFormatable(a->valueStart(),
                               a->valueLength(),
                               blockStart,
                               textLength,
                               formatable)) {
                setFormat(formatable.start, formatable.length, m_valueFormat);
              }
            }
          }
        }
        break;
      }
      case Node::End: {
        auto n = dynamic_cast<EndNode*>(node);
        if (n) {
          if (isFormatable(
                n->start(), n->length(), blockStart, textLength, formatable)) {
            setFormat(formatable.start, formatable.length, m_textFormat);
          }
          if (isFormatable(n->nameStart(),
                           n->nameLength(),
                           blockStart,
                           textLength,
                           formatable)) {
            setFormat(formatable.start, formatable.length, m_nameFormat);
          }
        }
        break;
      }
      case Node::CData: {
        auto n = dynamic_cast<CDataNode*>(node);
        if (n) {
          if (isFormatable(
                n->start(), n->length(), blockStart, textLength, formatable)) {
            setFormat(formatable.start, formatable.length, m_textFormat);
          }
          if (isFormatable(n->dataStart(),
                           n->dataLength(),
                           blockStart,
                           textLength,
                           formatable)) {
            setFormat(formatable.start, formatable.length, m_cdataFormat);
          }
        }
        break;
      }
      case Node::Instruction: {
        auto n = dynamic_cast<ProcessingInstruction*>(node);
        if (n) {
          if (isFormatable(
                n->start(), n->length(), blockStart, textLength, formatable)) {
            setFormat(formatable.start, formatable.length, m_textFormat);
          }
          if (isFormatable(n->targetStart(),
                           n->targetLength(),
                           blockStart,
                           textLength,
                           formatable)) {
            setFormat(formatable.start, formatable.length, m_piTargetFormat);
          }
          if (isFormatable(n->dataStart(),
                           n->dataLength(),
                           blockStart,
                           textLength,
                           formatable)) {
            setFormat(formatable.start, formatable.length, m_piDataFormat);
          }
        }
        break;
      }
      case Node::Comment: {
        auto n = dynamic_cast<CommentNode*>(node);
        if (n) {
          if (isFormatable(
                n->start(), n->length(), blockStart, textLength, formatable)) {
            setFormat(formatable.start, formatable.length, m_commentFormat);
          }
        }
        break;
      }
      default:
        break;
    }
  }
}

QColor
XmlHighlighter::xmlolor() const
{
  return m_xmlColor;
}

void
XmlHighlighter::setXmlolor(const QColor& Xmlolor)
{
  m_xmlColor = Xmlolor;
}

const QColor&
XmlHighlighter::piDataColor() const
{
  return m_piDataColor;
}

void
XmlHighlighter::setPiDataColor(const QColor& PiDataColor)
{
  m_piDataColor = PiDataColor;
}

const QColor&
XmlHighlighter::piTargetColor() const
{
  return m_piTargetColor;
}

void
XmlHighlighter::setPiTargetColor(const QColor& color)
{
  m_piTargetColor = color;
}

void
XmlHighlighter::setCdataColor(const QColor& color)
{
  m_cdataColor = color;
}

const QColor&
XmlHighlighter::cdataColor() const
{
  return m_cdataColor;
}

QColor
XmlHighlighter::textColor()
{
  return m_textColor;
}

void
XmlHighlighter::setTextColor(const QColor& color)
{
  m_textColor = color;
}

QColor
XmlHighlighter::background()
{
  return m_backgroundColor;
}

void
XmlHighlighter::setBackground(const QColor& color)
{
  m_backgroundColor = color;
}

QColor
XmlHighlighter::matchColor()
{
  return m_matchColor;
}

void
XmlHighlighter::setMatchColor(const QColor& color)
{
  m_matchColor = color;
}

QColor
XmlHighlighter::nameColor()
{
  return m_nameColor;
}

void
XmlHighlighter::setNameColor(const QColor& color)
{
  m_nameColor = color;
}

QColor
XmlHighlighter::attrColor()
{
  return m_attrColor;
}

void
XmlHighlighter::setAttrColor(const QColor& color)
{
  m_attrColor = color;
}

QColor
XmlHighlighter::valueColor()
{
  return m_valueColor;
}

void
XmlHighlighter::setValueColor(const QColor& color)
{
  m_valueColor = color;
}

QColor
XmlHighlighter::sQuoteColor()
{
  return m_sQuoteColor;
}

void
XmlHighlighter::setSQuoteColor(const QColor& color)
{
  m_sQuoteColor = color;
}

QColor
XmlHighlighter::dQuoteColor()
{
  return m_dQuoteColor;
}

void
XmlHighlighter::setDQuoteColor(const QColor& color)
{
  m_dQuoteColor = color;
}

QColor
XmlHighlighter::commentColor()
{
  return m_commentColor;
}

void
XmlHighlighter::setCommentColor(const QColor& color)
{
  m_commentColor = color;
}
