#include "qxml/xmleventparser.h"
#include "SMLibraries/utilities/characters.h"
#include "SMLibraries/utilities/filedownloader.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QThread>

//====================================================================
//=== XmlEventParser
//====================================================================
const QRegularExpression XmlEventParser::XMLDECL_REGEX =
  QRegularExpression("<\\?xml[^?]*\\?>",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::XML_REGEX =
  QRegularExpression("\\?xml",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::XMLDECL_PARTS_REGEX =
  QRegularExpression("(version\\s*=\\s*[\"']1\\.[01][\"'])|"
                     "(encoding\\s*=\\s*[\"']utf-[8|16][\"'])"
                     "|(standalone\\s*=\\s*[\"'](yes|no)[\"'])",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::VERSION_REGEX =
  QRegularExpression("version",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::ASSIGN_REGEX =
  QRegularExpression("=",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::VERSION_VALUE_REGEX =
  QRegularExpression("[\"']1\\.[01][\"']",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::ENCODING_REGEX =
  QRegularExpression("encoding",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::ENCODING_VALUE_REGEX =
  QRegularExpression("[\"']utf-[8|16][\"']",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);
const QRegularExpression XmlEventParser::STANDALONE_REGEX =
  QRegularExpression("standalone",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);

const QRegularExpression XmlEventParser::STANDALONE_VALUE_REGEX =
  QRegularExpression("[\"'](yes|no)[\"']",
                     QRegularExpression::CaseInsensitiveOption |
                       QRegularExpression::MultilineOption);

//====================================================================

XmlEventParser::XmlEventParser(QTextDocument* document, QObject* parent)
  : QObject{ parent }
  , m_document(document)
{
}

XmlEventParser::~XmlEventParser()
{
  if (m_rootNode) {
    delete (m_rootNode);
  }
}

bool
XmlEventParser::parseFile(QFile& file)
{
  if (file.exists()) {
    if (file.open(QFile::ReadWrite)) {
      QTextStream stream(&file);
      auto text = stream.readAll();
      parseString(text);
    }
  }
  return false;
}

bool
XmlEventParser::parseFile(const QString& filename)
{
  QFile file(filename);
  if (file.exists()) {
    if (file.open(QFile::ReadWrite)) {
      QTextStream stream(&file);
      auto text = stream.readAll();
      parseString(text);
    }
  }

  return true;
}

bool
XmlEventParser::parseString(const QString& text)
{
  parse_chunk(text.toStdString().c_str(), text.length());
  auto success = parse_finish();
  auto error = get_error_message();
  if (!success) {
    // OK not well formed so work through it.
    return false;
  }
  // detect xml declaration if any
  getXmlDeclaration(text);
  calculateNodePositions(text);
  return true;
}

bool
XmlEventParser::parseUrl(QUrl& url)
{
  if (url.isValid()) {
    if (url.isLocalFile()) {
      return parseFile(url.toLocalFile());
    } else {
      m_downloadCorrect = false;
      auto thread = new QThread();
      auto worker = new FileDownloader(url);
      worker->moveToThread(thread);
      connect(
        worker, &FileDownloader::error, this, &XmlEventParser::downloadError);
      connect(thread, &QThread::started, worker, &FileDownloader::download);
      connect(worker,
              &FileDownloader::dataDownloaded,
              this,
              &XmlEventParser::downloadComplete);
      connect(this, &XmlEventParser::finished, thread, &QThread::quit);
      connect(worker,
              &FileDownloader::finished,
              worker,
              &FileDownloader::deleteLater);
      connect(thread, &QThread::finished, thread, &QThread::deleteLater);
      thread->start();
      return m_downloadCorrect;
    }
  }
  return false;
}

void
XmlEventParser::getXmlDeclaration(const QString& text)
{
  QRegularExpressionMatch match = XMLDECL_REGEX.match(text);
  if (match.hasMatch()) {
    auto xml = new XmlDeclarationNode();
    auto xmltext = match.captured(0); // TODO remove
    auto pos = match.capturedStart(0);
    match = XML_REGEX.match(xmltext);
    if (match.hasMatch()) {
      xml->nameStartCursor = createCursor(pos + match.capturedStart(0));
    }
    xml->startCursor = createCursor(pos);
    xml->endCursor = createCursor(pos + xmltext.length());
    match = VERSION_REGEX.match(xmltext);
    if (match.hasMatch()) {
      xml->versionCursor = createCursor(pos + match.capturedStart(0));
    }
    match = ASSIGN_REGEX.match(xmltext, xml->versionStart());
    if (match.hasMatch()) {
      xml->versionAssign = createCursor(pos + match.capturedStart(0));
    }
    match = VERSION_VALUE_REGEX.match(xmltext, xml->versionAssignStart());
    if (match.hasMatch()) {
      xml->versionValueCursor = createCursor(pos + match.capturedStart(0));
      xml->version = match.captured(0);
    }

    match = ENCODING_REGEX.match(xmltext);
    if (match.hasMatch()) {
      xml->encodingCursor = createCursor(pos + match.capturedStart(0));
    }
    match = ASSIGN_REGEX.match(xmltext, xml->encodingStart());
    if (match.hasMatch()) {
      xml->encodingAssign = createCursor(pos + match.capturedStart(0));
    }
    match = ENCODING_VALUE_REGEX.match(xmltext, xml->encodingAssignStart());
    if (match.hasMatch()) {
      xml->encodingValueCursor = createCursor(pos + match.capturedStart(0));
      xml->encoding = match.captured(0);
    }

    match = STANDALONE_REGEX.match(xmltext);
    if (match.hasMatch()) {
      xml->standaloneCursor = createCursor(pos + match.capturedStart(0));
    }
    match = ASSIGN_REGEX.match(xmltext, xml->standaloneStart());
    if (match.hasMatch()) {
      xml->standaloneAssign = createCursor(pos + match.capturedStart(0));
    }
    match = STANDALONE_VALUE_REGEX.match(xmltext, xml->standaloneAssignStart());
    if (match.hasMatch()) {
      xml->standaloneValueCursor = createCursor(pos + match.capturedStart(0));
      xml->standalone = match.captured(0);
    }
    m_nodes.prepend(xml);
  }
}

void
XmlEventParser::calculateNodePositions(const QString& text)
{
  if (text.isEmpty())
    return;

  auto pos = 0;
  for (auto node : m_nodes) {
    switch (node->type) {
      case Node::Start: {
        auto start = dynamic_cast<StartNode*>(node);
        pos = text.indexOf(start->name, pos);
        start->nameStartCursor = createCursor(pos);
        start->startCursor = createCursor(reverseSearchForChar('<', text, pos));
        pos += start->nameLength();

        for (auto& attribute : start->attributes) {
          pos = text.indexOf(attribute->name, pos);
          attribute->nameStartCursor = createCursor(pos);
          pos += attribute->nameLength();
          pos = text.indexOf('=', pos);
          attribute->assignCursor = createCursor(pos);
          if (attribute->hasValue()) {
            pos = text.indexOf(attribute->value);
            attribute->valueStartCursor = createCursor(pos);
            pos += attribute->valueLength();
          }
        }

        pos = text.indexOf('>', pos);
        start->endCursor = createCursor(++pos);

        for (int i = start->startCursor.position();
             i < start->endCursor.position();
             ++i) {
          if (text.at(i) == Characters::NEWLINE) {
            start->newLines.append(i);
          }
        }

        break;
      }
      case Node::End: {
        auto end = dynamic_cast<EndNode*>(node);
        pos = text.indexOf(end->name, pos);
        end->nameStartCursor = createCursor(pos);
        end->startCursor = createCursor(reverseSearchForChar('<', text, pos));
        pos += end->nameLength();

        pos = text.indexOf('>', pos);
        end->endCursor = createCursor(++pos);

        for (int i = end->startCursor.position(); i < end->endCursor.position();
             ++i) {
          if (text.at(i) == Characters::NEWLINE) {
            end->newLines.append(i);
          }
        }

        break;
      }
      case Node::Text: {
        auto txt = dynamic_cast<TextNode*>(node);
        pos = text.indexOf(txt->text, pos);
        txt->startCursor = createCursor(pos);
        pos += txt->textLength();
        txt->endCursor = createCursor(pos);

        for (int i = txt->startCursor.position(); i < txt->endCursor.position();
             ++i) {
          if (text.at(i) == Characters::NEWLINE) {
            txt->newLines.append(i);
          }
        }

        break;
      }
      case Node::Comment: {
        auto comment = dynamic_cast<CommentNode*>(node);
        pos = text.indexOf(comment->comment, pos);
        comment->startCursor = createCursor(pos - 4); // <!--
        comment->commentStartCursor = createCursor(pos);
        pos += (comment->commentLength() + 3); // -->
        comment->endCursor = createCursor(pos);

        for (int i = comment->startCursor.position();
             i < comment->endCursor.position();
             ++i) {
          if (text.at(i) == Characters::NEWLINE) {
            comment->newLines.append(i);
          }
        }

        break;
      }
      case Node::CData: {
        auto comment = dynamic_cast<CDataNode*>(node);
        pos = text.indexOf(comment->data, pos);
        comment->startCursor = createCursor(pos - 9); // <![CDATA[
        comment->dataStartCursor = createCursor(pos);
        pos += (comment->dataLength() + 4); // ]]>
        comment->endCursor = createCursor(pos);

        for (int i = comment->startCursor.position();
             i < comment->endCursor.position();
             ++i) {
          if (text.at(i) == Characters::NEWLINE) {
            comment->newLines.append(i);
          }
        }

        break;
      }
      case Node::Instruction: {
        auto instruction = dynamic_cast<ProcessingInstruction*>(node);
        pos = text.indexOf(instruction->target, pos);
        instruction->targetStartCursor = createCursor(pos);
        instruction->startCursor =
          createCursor(reverseSearchForChar('<', text, pos));
        pos += (instruction->targetLength());
        pos = text.indexOf(instruction->data, pos);
        instruction->dataStartCursor = createCursor(pos);
        pos += instruction->dataLength();
        pos = text.indexOf(">", pos);
        instruction->endCursor = createCursor(pos + 1);

        for (int i = instruction->startCursor.position();
             i < instruction->endCursor.position();
             ++i) {
          if (text.at(i) == Characters::NEWLINE) {
            instruction->newLines.append(i);
          }
        }

        break;
      }
      default:
        break;
    }
  }
}

QTextCursor
XmlEventParser::createCursor(int position)
{
  auto cursor = QTextCursor(m_document);
  cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, position);
  return cursor;
}

int
XmlEventParser::reverseSearchForChar(QChar c, QString text, int searchFrom)
{
  for (int i = searchFrom; i >= 0; --i) {
    if (text.at(i) == c) {
      return i;
    }
  }
  return -1;
}

bool
XmlEventParser::isHaltOnError() const
{
  return m_haltOnError;
}

void
XmlEventParser::setHaltOnError(bool HaltOnError)
{
  m_haltOnError = HaltOnError;
}

const QMultiMap<QString, Node*>&
XmlEventParser::errors() const
{
  return m_errors;
}

Node*
XmlEventParser::rootNode() const
{
  return m_rootNode;
}

Node *XmlEventParser::nodeForPosition(int position) {
  for (auto node:m_nodes) {
    if (node->contains(position)) {
      return node;
    }
  }
  return nullptr;
}

const QVector<Node*>&
XmlEventParser::nodes() const
{
  return m_nodes;
}

bool
XmlEventParser::start_element(const std::string& name, const attrs_type& attrs)
{
  auto node = new StartNode(QString::fromStdString(name));
  for (const auto& [key, value] : attrs) {
    auto attr = new XmlAttribute(QString::fromStdString(key));
    if (!value.empty()) {
      attr->value = QString::fromStdString(value);
    }
    node->attributes.append(attr);
  }
  if (!m_rootNode) {
    m_rootNode = node;
    m_parentNode = m_rootNode;
  } else {
    m_parentNode->children.append(node);
    node->parent = m_parentNode;
    m_parentNode = node;
  }

  m_nodes.append(node);
  return true;
}

bool
XmlEventParser::end_element(const std::string& name)
{
  if (m_parentNode) {
    auto node = new EndNode(QString::fromStdString(name));
    auto parent = dynamic_cast<StartNode*>(m_parentNode);
    if (parent) {
      if (node->name != parent->name) {
        // TODO error start/end do not match
        node->errors.setFlag(Node::MismatchedNodes);
        auto errorMsg = tr("End node does not match start node");
        m_errors.insert(errorMsg, node);
        emit sendError(errorMsg);
        if (m_haltOnError) {
          return false;
        }
        return true;
      }
      parent->closer = node;
    }
    m_parentNode = m_parentNode->parent;
    m_nodes.append(node);
  }
  return true;
}

bool
XmlEventParser::text(const std::string& contents)
{
  auto node = new TextNode(QString::fromStdString(contents));
  node->parent = m_parentNode;
  if (m_parentNode) {
    m_parentNode->children.append(node);
  }
  m_nodes.append(node);
  return true;
}

bool
XmlEventParser::cdata(const std::string& contents)
{
  auto node = new CDataNode(QString::fromStdString(contents));
  node->parent = m_parentNode;
  if (m_parentNode) {
    m_parentNode->children.append(node);
  }
  m_nodes.append(node);
  return true;
}

bool
XmlEventParser::processing_instruction(const std::string& target,
                                       const std::string& data)
{
  auto node = new ProcessingInstruction(QString::fromStdString(target),
                                        QString::fromStdString(data));
  node->parent = m_parentNode;
  // processing instruction before first valid xml node.
  // have no parent level.
  if (m_parentNode) {
    m_parentNode->children.append(node);
  }
  m_nodes.append(node);
  return true;
}

bool
XmlEventParser::comment(const std::string& contents)
{
  auto node = new CommentNode(QString::fromStdString(contents));
  node->parent = m_parentNode;
  if (m_parentNode) {
    // covers comment outside root.
    m_parentNode->children.append(node);
  }
  m_nodes.append(node);
  return true;
}

bool
XmlEventParser::warning(const std::string& message)
{
  emit sendWarning(QString::fromStdString(message));
  return true;
}

void
XmlEventParser::downloadError(const QString& errorString)
{
  QString error(tr("A file download error has occurred %1").arg(errorString));
  emit sendError(error);
}

void
XmlEventParser::downloadComplete(const QByteArray& data)
{
  m_downloadCorrect = parseString(data);
  if (!m_downloadCorrect) {
    // TODO set some errors
    m_downloadCorrect = false;
  } else {
    m_downloadCorrect = true;
  }
  emit finished();
}

//====================================================================
//=== Attribute
//====================================================================
XmlAttribute::XmlAttribute() {}

XmlAttribute::XmlAttribute(const QString& name)
{
  this->name = name;
}

int
XmlAttribute::nameStart()
{
  return nameStartCursor.position();
}

int
XmlAttribute::nameLength()
{
  return name.length();
}

int
XmlAttribute::valueStart()
{
  return valueStartCursor.position();
}

int
XmlAttribute::valueLength()
{
  return value.length();
}

bool
XmlAttribute::hasValue()
{
  return !value.isEmpty();
}

XmlEventParser::XmlEventParser::IsInNodeType
XmlAttribute::isIn(int cursorPos)
{
  if (cursorPos >= nameStart() && cursorPos < nameStart() + name.length()) {
    return XmlEventParser::XmlEventParser::IsInAttributeName;
  } else if (cursorPos >= valueStart() &&
             cursorPos < valueStart() + value.length()) {
    return XmlEventParser::XmlEventParser::IsInAttributeValue;
  }
  return XmlEventParser::XmlEventParser::NotInNode;
}

//====================================================================
//=== Node. Holds common code.
//====================================================================
Node::Node() {}

Node::~Node() {}

XmlEventParser::XmlEventParser::IsInNodeType
Node::isIn(int cursorPos)
{
  if (cursorPos >= start() && cursorPos < end()) {
    return XmlEventParser::XmlEventParser::IsInNode;
  }
  return XmlEventParser::XmlEventParser::NotInNode;
}

int
Node::start()
{
  return startCursor.position();
}

int
Node::end()
{
  return endCursor.position();
}

int
Node::length()
{
  return end() - start();
}

void
Node::addSpaces(int n, QString& s)
{
  for (auto i = n; i > 0; --i) {
    s += Characters::SPACE;
  }
}

bool Node::contains(int position) {
  if (position >= start() && position < end()) return true;
  return false;
}

//====================================================================
//=== Node
//====================================================================
NameNode::NameNode() {}

NameNode::NameNode(const QString& name)
{
  this->name = name;
}

int
NameNode::nameStart()
{
  return nameStartCursor.position();
}

int
NameNode::nameLength()
{
  return name.length();
}

XmlEventParser::XmlEventParser::IsInNodeType
NameNode::isIn(int cursorPos)
{
  if (Node::isIn(cursorPos) == XmlEventParser::IsInNode) {
    if (cursorPos >= nameStart() && cursorPos < nameStart() + name.length()) {
      return XmlEventParser::IsInName;
    }
    return XmlEventParser::IsInNode;
  }
  return XmlEventParser::NotInNode;
}

//====================================================================
//=== TextNode
//====================================================================
TextNode::TextNode()
{
  type = Text;
}

TextNode::TextNode(const QString& text)
{
  this->text = text;
  type = Text;
}

QString
TextNode::toString()
{
  return text;
}

int
TextNode::textLength()
{
  return text.length();
}

XmlEventParser::IsInNodeType
TextNode::isIn(int cursorPos)
{
  if (Node::isIn(cursorPos) == XmlEventParser::IsInNode) {
    if (cursorPos >= start() && cursorPos < start() + text.length()) {
      return XmlEventParser::IsInText;
    }
    return XmlEventParser::IsInNode;
  }
  return XmlEventParser::NotInNode;
}

bool
TextNode::isWhitespace()
{
  return text.trimmed().isEmpty();
}

//====================================================================
//=== EndNode
//====================================================================
EndNode::EndNode()
{
  type = End;
}

EndNode::EndNode(const QString& name)
  : NameNode(name)
{
  type = End;
}

QString
EndNode::toString()
{
  QString s = "</";
  for (auto i = start() + 2; i < end() - 1; i++) {
    if (i < s.length())
      continue;
    if (i == nameStart()) {
      s += name;
      continue;
    }
    if (newLines.contains(i)) {
      s += Characters::NEWLINE;
      continue;
    }
    s += Characters::SPACE; // shouldn't happen in this tag type.
  }
  s += ">";
  return s;
}

//====================================================================
//=== StartNode
//====================================================================
StartNode::StartNode()
{
  type = Start;
}

StartNode::StartNode(const QString& name)
  : NameNode(name)
{
  type = Start;
}

StartNode::~StartNode()
{
  qDeleteAll(attributes);
}

XmlEventParser::IsInNodeType
StartNode::isIn(int cursorPos)
{
  auto result = NameNode::isIn(cursorPos);
  if (result == XmlEventParser::IsInNode) {
    for (auto i = 0; i < attributes.size(); i++) {
      auto d = attributes.at(i);
      auto attResult = d->isIn(cursorPos);
      if (attResult == XmlEventParser::IsInAttributeName ||
          attResult == XmlEventParser::IsInAttributeValue) {
        attributeIndex = i;
        return result;
      }
    }
    attributeIndex = -1;
    return result;
  } else if (result == XmlEventParser::IsInName) {
    return result;
  }
  return XmlEventParser::NotInNode;
}

QString
StartNode::toString()
{
  QString s = "<";
  for (auto i = start() + 1; i < end() - 1; i++) {
    if (i < s.length())
      continue;

    if (i == nameStart()) {
      s += name;
      i += name.length();
      continue;
    }

    if (newLines.contains(i)) {
      s += Characters::NEWLINE;
      continue;
    }

    for (auto att : attributes) {
      if (i == att->nameStart()) {
        s += att->name;
        i += att->name.length();
        break;
      }

      if (i == att->assignCursor.position()) {
        s += Characters::ASSIGNMENT;
        break;
      }

      if (!att->value.isEmpty() && i == att->valueStart()) {
        s += att->value;
        i += att->value.length();
        break;
        ;
      }
    }

    // fallback it's a space
    s.append(Characters::SPACE);
  }
  addSpaces(length() - s.length() - 1, s);
  s += ">";
  return s;
}

//====================================================================
//=== CommentNode
//====================================================================
CommentNode::CommentNode()
{
  type = Comment;
}

CommentNode::CommentNode(const QString& text)
  : comment(text)
{
  type = Comment;
}

int
CommentNode::commentStart()
{
  return commentStartCursor.position();
}

QString
CommentNode::toString()
{
  QString s = "<!--";
  addSpaces(commentStart() - start() - 4, s);
  s += comment;
  addSpaces(length() - s.length() - 4, s);
  s += "-->";
  return s;
}

XmlEventParser::IsInNodeType
CommentNode::isIn(int cursorPos)
{
  if (Node::isIn(cursorPos) == XmlEventParser::IsInNode) {
    if (cursorPos >= commentStart() &&
        cursorPos < commentStart() + comment.length()) {
      return XmlEventParser::IsInComment;
    }
    return XmlEventParser::IsInNode;
  }
  return XmlEventParser::NotInNode;
}

int
CommentNode::commentLength()
{
  return comment.length();
}

//====================================================================
//=== ProcessingInstruction
//====================================================================
ProcessingInstruction::ProcessingInstruction()
{
  type = Instruction;
}

ProcessingInstruction::ProcessingInstruction(const QString target,
                                             const QString data)
{
  type = Instruction;
  this->target = target;
  this->data = data;
}

int
ProcessingInstruction::targetStart()
{
  return targetStartCursor.position();
}

int
ProcessingInstruction::targetLength()
{
  return target.length();
}

int
ProcessingInstruction::dataStart()
{
  return dataStartCursor.position();
}

int
ProcessingInstruction::dataLength()
{
  return data.length();
}

QString
ProcessingInstruction::toString()
{
  QString s = "<?";
  addSpaces(targetStart() - start() - 2, s);
  s += target;
  addSpaces(dataStart() - targetStart() - target.length(), s);
  s += data;
  addSpaces(length() - s.length() - 2, s);
  s += "?>";
  return s;
}

XmlEventParser::IsInNodeType
ProcessingInstruction::isIn(int cursorPos)
{
  auto result = Node::isIn(cursorPos);
  if (result == XmlEventParser::IsInNode) {
    if (cursorPos >= targetStart() &&
        cursorPos < targetStart() + target.length()) {
      return XmlEventParser::IsInPITarget;
    }
    if (cursorPos >= dataStart() && cursorPos < dataStart() + data.length()) {
      return XmlEventParser::IsInPIData;
    }
    return result;
  }
  return XmlEventParser::NotInNode;
}

//====================================================================
//=== CDataNode
//====================================================================
CDataNode::CDataNode()
{
  type = CData;
}

CDataNode::CDataNode(const QString& data)
{
  type = CData;
  this->data = data;
}

int
CDataNode::dataStart()
{
  return dataStartCursor.position();
}

QString
CDataNode::toString()
{
  QString s = "<!CDATA[";
  addSpaces(dataStart() - start() - 8, s);
  s += data;
  addSpaces(length() - s.length() - 3, s);
  s += "]]>";
  return s;
}

XmlEventParser::IsInNodeType
CDataNode::isIn(int cursorPos)
{
  if (Node::isIn(cursorPos) == XmlEventParser::IsInNode) {
    if (cursorPos >= dataStart() && cursorPos < dataStart() + data.length()) {
      return XmlEventParser::IsInComment;
    }
    return XmlEventParser::IsInNode;
  }
  return XmlEventParser::NotInNode;
}

int
CDataNode::dataLength()
{
  return data.length();
}

//====================================================================
//=== XmlDeclarationNode
//====================================================================
bool
XmlDeclarationNode::hasVersion()
{
  return !version.isEmpty();
}

bool
XmlDeclarationNode::hasEncoding()
{
  return !encoding.isEmpty();
}

bool
XmlDeclarationNode::hasStandalone()
{
  return !standalone.isEmpty();
}

int
XmlDeclarationNode::versionStart()
{
  return versionCursor.position();
}

int
XmlDeclarationNode::versionAssignStart()
{
  return versionAssign.position();
}

int
XmlDeclarationNode::versionValueStart()
{
  return versionValueCursor.position();
}

int
XmlDeclarationNode::encodingStart()
{
  return encodingCursor.position();
}

int
XmlDeclarationNode::encodingAssignStart()
{
  return encodingAssign.position();
}

int
XmlDeclarationNode::encodingValueStart()
{
  return encodingValueCursor.position();
}

int
XmlDeclarationNode::standaloneStart()
{
  return standaloneCursor.position();
}

int
XmlDeclarationNode::standaloneAssignStart()
{
  return standaloneAssign.position();
}

int
XmlDeclarationNode::standaloneValueStart()
{
  return standaloneValueCursor.position();
}
