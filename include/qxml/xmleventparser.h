#pragma once

#include <QFile>
#include <QMap>
#include <QObject>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QThread>
#include <QUrl>

#include <xmlwrapp/event_parser.h>

struct XmlAttribute;
struct Node;
struct NameNode;
struct StartNode;
struct EndNode;
struct TextNode;
struct CDataNode;
struct CommentNode;
struct ProcessingInstruction;

/*!
 * \ingroup widgets
 * \class XmlEventParser xmleventparser.h
 * "include/SMLibraries/widgets/xmleventparser.h" \brief A Qt wrapper for
 * XmlWrapp event_parser.
 *
 * XmlWrapp is a C++ wrapper for the C libxml2 XML parsing library.
 * XmlEventParser is a Qt wrapper for the XmlWrapp event_parser part of the
 * XmlWrapp library.
 *
 * Use parseFile(const QFile&), parseFile(const QString&), parseString(const
 * QString&) or parseUrl(const QUrl&) to parse the xml data.
 *
 * By default XmlWrapp, and XmlEventParser, halts parsing when an error is
 * detected. Set the haltOnError flag, setHaltOnError(false), if you want to
 * complete parsing before handling errors. You can access the complete set of
 * errors by accessing via errors() which returns a QMultiMap<QString,
 * BaseNode*> of error strings => the node causing the problem.
 *
 * The positioning of the various start/end points are as below.
 * \code
 *  ⭣ node start
 *   ⭣ name start
 *        ⭣ attribute start
 *                    ⭣ value start
 *                         ⭣ node end
 *  <name attribute = value>
 * \endcode
 */
class XmlEventParser
  : public QObject
  , public xml::event_parser
{
  Q_OBJECT
public:
  /*!
   * \enum  XmlEventParser::IsInNodeType
   *
   * A value returned by the Node::isIn method.
   */
  enum IsInNodeType
  {
    NotInNode,          //!< Not in the node
    IsInNode,           //!< Is in the node
    IsInAttributeName,  //!< Is in the attribute node
    IsInAttributeValue, //!< Is in the attribute node
    IsInName,           //!< is in the tag node
    //    IsInXmlDocTypeData,    //!< Is in the DocType data
    IsInText,     //!< Is in the text block
    IsInComment,  //!< Is in the comment
    IsInPITarget, //!< Is in the processing instruction target
    IsInPIData,   //!< Is in the processing instruction data
  };

  explicit XmlEventParser(QTextDocument* document, QObject* parent = nullptr);
  ~XmlEventParser();

  //! \brief Parses the file specified by the QFile object if it exists.
  //!
  //! Returns true if the parser encounters no errors, otherwise returns false.
  //!
  bool parseFile(QFile& file);

  //! \brief Parses the file specified by the QString filename if it exists.
  //!
  //! Returns true if the parser encounters no errors, otherwise returns false.
  //!
  bool parseFile(const QString& filename);

  //! \brief Parses the text string.
  //!
  //! Returns true if the parser encounters no errors, otherwise returns false.
  //!
  bool parseString(const QString& text);

  //!
  //! \brief Parses the url specified by the network QUrl if it exists.
  //!
  //! Returns true if the parser encounters no errors, otherwise returns false.
  //!
  bool parseUrl(QUrl& url);

  bool isHaltOnError() const;
  void setHaltOnError(bool HaltOnError);

  const QMultiMap<QString, Node*>& errors() const;

  Node* rootNode() const;
  Node *nodeForPosition(int position);
  const QVector<Node*>& nodes() const;

signals:
  void sendError(const QString&);
  void sendWarning(const QString&);
  void finished();

protected:
  QTextDocument* m_document;
  QMultiMap<QString, Node*> m_errors;
  Node* m_rootNode = nullptr;
  Node* m_parentNode = nullptr;
  QVector<Node*> m_nodes;
  bool m_haltOnError = true;
  bool m_downloadCorrect = false;

  bool start_element(const std::string& name, const attrs_type& attrs);
  bool end_element(const std::string& name);
  bool text(const std::string& contents);
  bool cdata(const std::string& contents);
  bool processing_instruction(const std::string& target,
                              const std::string& data);
  bool comment(const std::string& contents);
  bool warning(const std::string& message);

  void downloadError(const QString& errorString);
  void downloadComplete(const QByteArray& data);

private:
  QTextCursor createCursor(int position);
  int reverseSearchForChar(QChar c, QString text, int searchFrom);
  void calculateNodePositions(const QString& text);

  static const QRegularExpression XMLDECL_REGEX;
  static const QRegularExpression XML_REGEX;
  static const QRegularExpression XMLDECL_PARTS_REGEX;
  static const QRegularExpression VERSION_REGEX;
  static const QRegularExpression ASSIGN_REGEX;
  static const QRegularExpression VERSION_VALUE_REGEX;
  static const QRegularExpression ENCODING_REGEX;
  static const QRegularExpression ENCODING_VALUE_REGEX;
  static const QRegularExpression STANDALONE_REGEX;
  static const QRegularExpression STANDALONE_VALUE_REGEX;

  void getXmlDeclaration(const QString& text);
};

struct XmlAttribute
{
  XmlAttribute();
  XmlAttribute(const QString& name);

  int nameStart();

  int nameLength();

  int valueStart();

  int valueLength();

  bool hasValue();

  XmlEventParser::XmlEventParser::IsInNodeType isIn(int cursorPos);

  //! The tag name
  QString name;
  //! The start position of the tag
  QTextCursor nameStartCursor;
  //! The attribute value
  QString value;
  //! The end position of the tag
  QTextCursor valueStartCursor;
  //! allows for gaps between name and assignment =
  QTextCursor assignCursor;
};

//! \struct Node
struct Node
{
  enum Type
  {
    Base,
    XmlDeclaration,
    Text,
    Start,
    End,
    CData,
    Instruction,
    Comment,
  };
  enum Error
  {
    NoError = 0x0,
    MismatchedNodes = 0x1,
  };
  Q_DECLARE_FLAGS(Errors, Error)

  Node();
  virtual ~Node();

  //! Indicates that the node has child nodes, true if child nodes
  //! exist, otherwise false;
  bool hasChildren();

  /*!
   * \brief isIn method of all Tag types.
   *
   * Returns the result of a test whether the cursor is
   * within the tag. Returns one of the IsInType values.
   */
  virtual XmlEventParser::IsInNodeType isIn(int cursorPos);

  /*!
   * \brief Creates a string version of the tag.
   *
   * This is an empty virtual method in Tag.
   */
  virtual QString toString() = 0;

  //! \brief The start position of the BaseNode in the text
  int start();

  //! \brief The end position of the BaseNode in the text
  int end();

  /*!
   * \brief Returns the tag length.
   */
  int length();

  //! Adds n spaces to string s.
  static void addSpaces(int n, QString& s);

  bool contains(int position);

  Node* parent = nullptr;
  //! The child nodes of this nodes.
  QVector<Node*> children;
  //! The QTextCursor at the start position of the tag.
  QTextCursor startCursor;
  //! The QTextCursor at the end position of the tag.
  QTextCursor endCursor;
  //! The node type.
  Type type = Base;
  //! The errors generated by the process.
  Errors errors = NoError;
  //! Stores newlines inside tags.
  QList<int> newLines;
};

struct NameNode : Node
{
  NameNode();
  NameNode(const QString& name);

  /*!
   * \brief Returns the tag name start position.
   */
  int nameStart();
  /*!
   * \brief Returns the length of the text.
   */
  int nameLength();

  /*!
   * \brief isIn method of all name nodes types.
   *
   * Returns the result of a test whether the cursor is
   * within the tag. Returns one of the IsInType values.
   */
  XmlEventParser::IsInNodeType isIn(int cursorPos) override;

  //! The QTextCursor at the start position of the tag name.
  QTextCursor nameStartCursor;
  //! The tag name.
  QString name;
};

struct XmlDeclarationNode : NameNode
{
  XmlDeclarationNode() { type = XmlDeclaration; }

  /*!
   * \brief Creates a string version of the tag.
   *
   * Override to create a string version of derived tags.
   */
  QString toString() override
  {
    // TODO
    return QString();
  }

  bool hasVersion();
  bool hasEncoding();
  bool hasStandalone();
  int versionStart();
  int versionAssignStart();
  int versionValueStart();
  int encodingStart();
  int encodingAssignStart();
  int encodingValueStart();
  int standaloneStart();
  int standaloneAssignStart();
  int standaloneValueStart();

  QTextCursor versionCursor;
  QTextCursor versionAssign;
  QTextCursor versionValueCursor;
  QString version;
  QTextCursor encodingCursor;
  QTextCursor encodingAssign;
  QTextCursor encodingValueCursor;
  QString encoding;
  QTextCursor standaloneCursor;
  QTextCursor standaloneAssign;
  QTextCursor standaloneValueCursor;
  QString standalone;
};


struct StartNode : NameNode
{
  StartNode();
  StartNode(const QString& name);
  ~StartNode();

  XmlEventParser::IsInNodeType isIn(int cursorPos) override;

  /*!
   * \brief Creates a string version of the tag.
   *
   * Override to create a string version of derived tags.
   */
  QString toString() override;

  //! The index of the attribute that has been detected by the IsIn method.
  int attributeIndex;
  //! List of attributes.
  QVector<XmlAttribute*> attributes;
  //! The closer node
  Node* closer;
};

struct EndNode : NameNode
{
  EndNode();
  EndNode(const QString& name);

  QString toString() override;
};

struct TextNode : Node
{
  TextNode();
  TextNode(const QString& text);

  /*!
   * \brief Creates a string version of the tag.
   *
   * Override to create a string version of derived tags.
   */
  QString toString() override;

  /*!
   * \brief Returns the length of the text string.
   *
   * Equivalent of calling text().length().
   */
  int textLength();

  /*!
   * \brief isIn method of all Tag types.
   *
   * Returns the result of a test whether the cursor is
   * within the tag. Returns one of the IsInType values.
   */
  XmlEventParser::IsInNodeType isIn(int cursorPos) override;

  //! Returns true if the string only contains whitespace characters.
  //!
  //! This includes  \code \t, \n, \v, \f and \r \endcode characters as well as
  //! space characters.
  bool isWhitespace();

  //! The QTextCursor at the start position of the tag name.
  QTextCursor textStartCursor;
  /*!
   * \brief the text string
   */
  QString text;
};

struct CDataNode : Node
{
  CDataNode();
  CDataNode(const QString& data);

  int dataStart();

  QString toString() override;

  XmlEventParser::IsInNodeType isIn(int cursorPos) override;

  int dataLength();

  //! The QTextCursor at the start position of the tag text.
  QTextCursor dataStartCursor;
  QString data;
};

struct CommentNode : Node
{
  CommentNode();
  CommentNode(const QString& text);

  int commentStart();

  /*!
   * \brief Returns the length of the text string.
   *
   * Equivalent of calling text().length().
   */
  int commentLength();

  QString toString() override;

  XmlEventParser::IsInNodeType isIn(int cursorPos) override;

  //! Returns true if the string only contains whitespace characters.
  //!
  //! This includes  \code \t, \n, \v, \f and \r \endcode characters as well as
  //! space characters.
  bool isWhitespace() { return comment.trimmed().isEmpty(); }

  //! The QTextCursor at the start position of the tag name.
  QTextCursor commentStartCursor;

  /*!
   * \brief the text string
   */
  QString comment;
};

struct ProcessingInstruction : Node
{
  ProcessingInstruction();
  ProcessingInstruction(const QString target, const QString data);

  int targetStart();
  int targetLength();

  int dataStart();
  int dataLength();

  QString toString() override;

  XmlEventParser::IsInNodeType isIn(int cursorPos) override;

  QTextCursor targetStartCursor;
  QTextCursor dataStartCursor;

  QString target;
  QString data;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Node::Errors)
