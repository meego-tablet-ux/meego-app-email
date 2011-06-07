#include "htmlfield.h"
#include <QDeclarativeEngine>
#include <QWebFrame>

HtmlField::HtmlField(QDeclarativeItem *parent) : QDeclarativeItem(parent)
{
    init();
}

HtmlField::~HtmlField()
{
    delete m_gwv;
}

void HtmlField::init()
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemHasNoContents, true);
    setClip(true);

    m_gwv = new QGraphicsWebView(this);
    m_gwv->setResizesToContents(true);

    QWebPage* page = new QWebPage(this);
    m_gwv->setPage(page);
    connect(page,SIGNAL(contentsChanged()),this,SIGNAL(modifiedChanged()));
    connect(page,SIGNAL(contentsChanged()),this,SIGNAL(htmlChanged()));

    setDelegateLinks(true);
    m_gwv->setAcceptTouchEvents(false);
    m_gwv->setAcceptedMouseButtons(Qt::LeftButton);

    setFocusProxy(m_gwv);

    page->mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    page->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    page->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, true);

    connect(page->mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SIGNAL(contentsSizeChanged(QSize)));
    connect(m_gwv,  SIGNAL(geometryChanged()),       this, SLOT(webViewUpdateImplicitSize()));
    connect(m_gwv,  SIGNAL(scaleChanged()),          this, SIGNAL(contentsScaleChanged()));
    connect(m_gwv,  SIGNAL(linkClicked(QUrl)),       this, SIGNAL(linkClicked(QUrl)));
}

void HtmlField::startZooming()
{
    m_gwv->setAcceptedMouseButtons(Qt::NoButton);
    setTiledBackingStoreFrozen(true);
}
void HtmlField::stopZooming()
{
    setTiledBackingStoreFrozen(false);
    m_gwv->setAcceptedMouseButtons(Qt::LeftButton);
}

void HtmlField::componentComplete()
{
    QDeclarativeItem::componentComplete();
    m_gwv->page()->setNetworkAccessManager(qmlEngine(this)->networkAccessManager());

}

bool HtmlField::delegateLinks() const
{
    return (m_gwv->page()->linkDelegationPolicy() == QWebPage::DelegateAllLinks);
}

void HtmlField::setDelegateLinks(bool f)
{
    if (f != delegateLinks()) {
        m_gwv->page()->setLinkDelegationPolicy(f ? QWebPage::DelegateAllLinks : QWebPage::DontDelegateLinks);
        emit delegateLinksChanged();
    }
}

void HtmlField::webViewUpdateImplicitSize()
{
    QSizeF size = m_gwv->geometry().size() * contentsScale();
    setImplicitWidth(size.width());
    setImplicitHeight(size.height());
}

void HtmlField::updateContentsSize()
{
    if (m_gwv->page()) {
        m_gwv->page()->setPreferredContentsSize(m_gwv->preferredSize().toSize());
    }
}

void HtmlField::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QWebPage* webPage = m_gwv->page();
    if (webPage && newGeometry.size() != oldGeometry.size()) {
        QSize contentSize = webPage->preferredContentsSize();
        if (widthValid())
            contentSize.setWidth(width());
        if (heightValid())
            contentSize.setHeight(height());
        if (contentSize != webPage->preferredContentsSize())
            webPage->setPreferredContentsSize(contentSize);
    }
    QDeclarativeItem::geometryChanged(newGeometry, oldGeometry);
}

bool HtmlField::editable() const
{
    return (m_gwv->page()) ? m_gwv->page()->isContentEditable() : false;
}

void HtmlField::setEditable(bool enabled)
{
    if (m_gwv->page() && m_gwv->page()->isContentEditable() != enabled) {
        m_gwv->page()->setContentEditable(enabled);
        emit editableChanged();
    }
}

bool HtmlField::modified() const
{
    return (m_gwv->page()) ? m_gwv->page()->isModified() : false;
}

bool HtmlField::tiledBackingStoreFrozen() const
{
    return m_gwv->isTiledBackingStoreFrozen();
}

void HtmlField::setTiledBackingStoreFrozen(bool enabled)
{
    if (enabled != m_gwv->isTiledBackingStoreFrozen()) {
        m_gwv->setTiledBackingStoreFrozen(enabled);
        emit tiledBackingStoreFrozenChanged();
    }
}

QString HtmlField::html() const
{
    return m_gwv->page()->mainFrame()->toHtml();
}

void HtmlField::setHtml(const QString& html)
{
    m_gwv->setHtml(html);
    updateContentsSize();
    emit htmlChanged();
}

QSize HtmlField::contentsSize() const
{
    return m_gwv->page()->mainFrame()->contentsSize() * contentsScale();
}

qreal HtmlField::contentsScale() const
{
    return m_gwv->scale();
}

void HtmlField::setContentsScale(qreal scale)
{
    if (scale != m_gwv->scale()) {
        m_gwv->setScale(scale);
        webViewUpdateImplicitSize();
        emit contentsScaleChanged();
    }
}


