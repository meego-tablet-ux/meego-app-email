#ifndef EMAIL_HTMLFIELD_H
#define EMAIL_HTMLFIELD_H

#include <QtDeclarative/QDeclarativeItem>
#include <QGraphicsWebView>
#include <QTimer>

class HFWebView;

class HtmlField : public QDeclarativeItem {
    Q_OBJECT

    Q_PROPERTY(QString html READ html WRITE setHtml NOTIFY htmlChanged)

    // Sets the entire contents editiable See QWebPage::setContentEditable()
    // NOTE: If you want to set just part of it editable you can do that by wrapping
    // that in the HTML itself by wrapping it in "<DIV CONTENTEDITABLE> EDIT ME... </DIV>"
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)

    Q_PROPERTY(bool tiledBackingStoreFrozen READ tiledBackingStoreFrozen WRITE setTiledBackingStoreFrozen NOTIFY tiledBackingStoreFrozenChanged)
    Q_PROPERTY(qreal contentsScale READ contentsScale WRITE setContentsScale NOTIFY contentsScaleChanged)
    Q_PROPERTY(bool delegateLinks READ delegateLinks WRITE setDelegateLinks NOTIFY delegateLinksChanged);
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)

    Q_PROPERTY(int contentsTimeoutMs READ contentsTimeoutMs WRITE setContentsTimeoutMs NOTIFY contentsTimeoutMsChanged);

    Q_PROPERTY(int preferredWidth READ preferredWidth WRITE setPreferredWidth NOTIFY preferredWidthChanged)
    Q_PROPERTY(int preferredHeight READ preferredHeight WRITE setPreferredHeight NOTIFY preferredHeightChanged)

public:

    HtmlField(QDeclarativeItem *parent = 0);
    ~HtmlField();

    Q_INVOKABLE bool setFocusElement(const QString& elementName);
    Q_INVOKABLE void startZooming();
    Q_INVOKABLE void stopZooming();

    Q_INVOKABLE void forceFocus();
    Q_INVOKABLE void openSoftwareInputPanel();
    Q_INVOKABLE void closeSoftwareInputPanel();

    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);

    Q_INVOKABLE bool copyAvailable() const;
    Q_INVOKABLE void copy();

    Q_INVOKABLE bool cutAvailable() const;
    Q_INVOKABLE void cut();

    Q_INVOKABLE bool pasteAvailable() const;
    Q_INVOKABLE void paste();

    Q_INVOKABLE void selectWord(const QPoint &pos);
    Q_INVOKABLE void selectNextWord();
    Q_INVOKABLE void selectPrevWord();    
    Q_INVOKABLE void selectAll();

    Q_INVOKABLE bool hasSelection() const { return selectedText() != ""; }
    Q_INVOKABLE QString selectedText() const;
    Q_INVOKABLE bool isSelected(const QPoint &pos) const;


    // Property Getters/Setters
    QString html() const;
    void setHtml(const QString &html);

    bool readOnly() const;
    void setReadOnly(bool);
    bool modified() const;

    void setDelegateLinks(bool f);
    bool delegateLinks() const;

    bool tiledBackingStoreFrozen() const;
    void setTiledBackingStoreFrozen(bool f);

    int contentsTimeoutMs() const;
    void setContentsTimeoutMs(int msec);


public:
    int preferredWidth() const;
    void setPreferredWidth(int);
    int preferredHeight() const;
    void setPreferredHeight(int);

    QSize contentsSize() const;
    void setContentsScale(qreal scale);
    qreal contentsScale() const;

    void setFont(const QFont & scale);
    QFont font() const;

signals:
    void linkClicked ( const QUrl & url );

    void readOnlyChanged();
    void modifiedChanged();
    void tiledBackingStoreFrozenChanged();
    void htmlChanged();
    void contentsSizeChanged(const QSize&);
    void contentsScaleChanged();
    void zoomFactorChanged();

    void fontChanged();
    void delegateLinksChanged();
    void contentsTimeoutMsChanged();

    void loadFinished(bool ok);
    void loadProgress(int progress);
    void loadStarted();
    void statusBarMessage(const QString & message);

    void preferredWidthChanged();
    void preferredHeightChanged();
    void preferredSizeChanged();

private slots:
    void webViewUpdateImplicitSize();
    void updateContentsSize();

    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);

    void privateOnLoadProgress(int progress);
    void privateOnLoadFinished(bool ok);
    void privateOnLoadStarted();

    void privateOnContentTimout();

private:
    HFWebView *m_gwv;
    QTimer m_loadTimer;
    int m_preferredWidth;
    int m_preferredHeight;

    void init();
    virtual void componentComplete();
    Q_DISABLE_COPY(HtmlField)

};

class HFWebView: public QGraphicsWebView
{
    Q_OBJECT
public:
    HFWebView(QGraphicsItem *parent);
    void keyPressEvent(QKeyEvent *);
    void showContextMenu();
    void selectWord(const QPointF &pos);


    QWebHitTestResult hitTestContent( const QPoint &pos ) const;
};

#endif
