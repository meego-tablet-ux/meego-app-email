#ifndef EMAIL_HTMLFIELD_H
#define EMAIL_HTMLFIELD_H

#include <QtDeclarative/QDeclarativeItem>
#include <QGraphicsWebView>

class HtmlField : public QDeclarativeItem {
    Q_OBJECT

    Q_PROPERTY(QString html READ html WRITE setHtml NOTIFY htmlChanged)

    // Sets the entire contents editiable See QWebPage::setContentEditable()
    // NOTE: If you want to set just part of it editable you can do that by wrapping
    // that in the HTML itself by wrapping it in "<DIV CONTENTEDITABLE> EDIT ME... </DIV>"
    Q_PROPERTY(bool editable READ editable WRITE setEditable NOTIFY editableChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)

    Q_PROPERTY(bool tiledBackingStoreFrozen READ tiledBackingStoreFrozen WRITE setTiledBackingStoreFrozen NOTIFY tiledBackingStoreFrozenChanged)
    Q_PROPERTY(qreal contentsScale READ contentsScale WRITE setContentsScale NOTIFY contentsScaleChanged)
    Q_PROPERTY(bool delegateLinks READ delegateLinks WRITE setDelegateLinks NOTIFY delegateLinksChanged);

public:

    Q_INVOKABLE void startZooming();
    Q_INVOKABLE void stopZooming();

    HtmlField(QDeclarativeItem *parent = 0);
    ~HtmlField();

    QString html() const;
    void setHtml(const QString &html);

    bool editable() const;
    void setEditable(bool);
    bool modified() const;

    void setDelegateLinks(bool f);
    bool delegateLinks() const;

    bool tiledBackingStoreFrozen() const;
    void setTiledBackingStoreFrozen(bool f);

public:

    QSize contentsSize() const;
    void setContentsScale(qreal scale);
    qreal contentsScale() const;

signals:
    void linkClicked ( const QUrl & url );

    void editableChanged();
    void modifiedChanged();
    void tiledBackingStoreFrozenChanged();
    void htmlChanged();
    void contentsSizeChanged(const QSize&);
    void contentsScaleChanged();
    void delegateLinksChanged();

private slots:
    void webViewUpdateImplicitSize();
    void updateContentsSize();

    virtual void geometryChanged(const QRectF &newGeometry,
                                 const QRectF &oldGeometry);

private:
    QGraphicsWebView *m_gwv;
    void init();
    virtual void componentComplete();
    Q_DISABLE_COPY(HtmlField)
};


#endif
