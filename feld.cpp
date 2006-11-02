/****************************************************************
**
** Implementation Feld class, derieved from Qt tutorial 8
**
****************************************************************/

// bemerkungen : wenn paintEvent aufgerufen wird, wird das komplette
//               widget gelöscht und nur die sachen gezeichnet, die in
//               paintEvent stehen ! sollen dinge z.b nur bei maustasten-
//               druck gezeichnet werden, so muß dies in mousePressEvent
//               stehen !
//               paintEvent wird aufgerufen, falls fenster überdeckt wird,
//               oder auch einfach bewegt wird

#include <kiconloader.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>
#include <QAbstractEventDispatcher>
#include <QPainter>
#include <QMouseEvent>
#include <QTimerEvent>
#include <QPaintEvent>
#include "molek.h"
#include "feld.h"
#include "settings.h"
#include "katomicrenderer.h"

#if FIELD_SIZE < MOLEK_SIZE
#error Molecule size (MOLEK_SIZE) must be <= field size (FIELD_SIZE)
#endif

extern Options settings;

Feld::Feld( QWidget *parent )
    : QWidget( parent ), undoBegin (0), undoSize (0), redoSize (0)
{
    m_elemSize = 30;
    anim = false;
    dir = None;
    sprite = QPixmap(m_elemSize, m_elemSize);

    cx = -1;
    cy = -1;

    moving = false;
    chosen = false;

    setMouseTracking(true);

    setFocusPolicy(Qt::StrongFocus);

    m_renderer = new KAtomicRenderer( KStandardDirs::locate("appdata", "pics/abilder.svgz"), this );
    m_renderer->setElementSize( m_elemSize );
    resize(FIELD_SIZE * m_elemSize, FIELD_SIZE * m_elemSize);
    copy = QPixmap(FIELD_SIZE * m_elemSize, FIELD_SIZE * m_elemSize);
}

Feld::~Feld ()
{
}

void Feld::resetValidDirs()
{
    bool up = false;
    for (int j = 0; j < FIELD_SIZE; j++)
        for (int i = 0; i < FIELD_SIZE; i++)
            if (feld[i][j] >= 150 && feld[i][j] <= 153)
            {
                feld[i][j] = 0;
                up = true;
            }
    if (up) update();
}

void Feld::load (const KSimpleConfig& config)
{
    if(moving)
        QAbstractEventDispatcher::instance()->unregisterTimers(this);

    mol->load(config);

    QString key;

    for (int j = 0; j < FIELD_SIZE; j++) {

        key.sprintf("feld_%02d", j);
        QString line = config.readEntry(key,QString());

        for (int i = 0; i < FIELD_SIZE; i++)
            feld[i][j] = atom2int(line[i].toLatin1());

    }

    moves = 0;
    chosen = false;
    moving = false;

    undoSize = redoSize = undoBegin = 0;
    emit enableUndo(false);
    emit enableRedo(false);

    xpos = ypos = 0;
    nextAtom();
}

void Feld::mousePressEvent (QMouseEvent *e)
{
    if (moving)
        return;

    int x = e->pos ().x () / m_elemSize;
    int y = e->pos ().y () / m_elemSize;

    if ( feld [x] [y] == 150)
        startAnimation (Feld::MoveUp);
    else if ( feld [x] [y] == 151)
        startAnimation (Feld::MoveLeft);
    else if ( feld [x] [y] == 152)
        startAnimation (Feld::MoveDown);
    else if ( feld [x] [y] == 153)
        startAnimation (Feld::MoveRight);
    else if (feld [x] [y] != 254 && feld [x] [y] != 0) {
        chosen = true;
        xpos = x;
        ypos = y;
        dir = None;
        resetValidDirs();
    } else {
        resetValidDirs();
        chosen = false;
    }
    emitStatus();
}

const atom& Feld::getAtom(uint index) const
{
    return mol->getAtom(index);
}


void Feld::nextAtom()
{
    int x = xpos, y;

    // make sure we don't check the current atom :-)
    if (ypos++ >= FIELD_SIZE) ypos = 0;

    while(1)
    {
        for (y = ypos; y < FIELD_SIZE; y++)
        {
            if ( feld [x] [y] != 0 &&
                    feld [x] [y] != 254 &&
                    feld [x] [y] != 150 &&
                    feld [x] [y] != 151 &&
                    feld [x] [y] != 152 &&
                    feld [x] [y] != 153 )
            {
                xpos = x; ypos = y;
                chosen = true;
                resetValidDirs();
                emitStatus();
                return;
            }
        }
        ypos = 0;
        x++;
        if (x >= FIELD_SIZE) x = 0;
    }

}


void Feld::previousAtom()
{
    int x = xpos, y;

    // make sure we don't check the current atom :-)
    if (ypos-- <= 0) ypos = FIELD_SIZE-1;

    while(1)
    {
        for (y = ypos; y >= 0; y--)
        {
            if ( feld [x] [y] != 0 &&
                    feld [x] [y] != 254 &&
                    feld [x] [y] != 150 &&
                    feld [x] [y] != 151 &&
                    feld [x] [y] != 152 &&
                    feld [x] [y] != 153 )
            {
                xpos = x; ypos = y;
                chosen = true;
                resetValidDirs();
                emitStatus();
                return;
            }
        }
        ypos = FIELD_SIZE-1;
        x--;
        if (x <= 0) x = FIELD_SIZE-1;
    }
}


void Feld::emitStatus()
{
    if (!chosen || moving) {}
    else {

        if (ypos > 0 && feld[xpos][ypos-1] == 0) {
            feld [xpos][ypos-1] = 150;
            update();
        }

        if (ypos < FIELD_SIZE-1 && feld[xpos][ypos+1] == 0) {
            feld [xpos][ypos+1] = 152;
            update();
        }

        if (xpos > 0 && feld[xpos-1][ypos] == 0) {
            feld [xpos-1][ypos] = 151;
            update();
        }

        if (xpos < FIELD_SIZE-1 && feld[xpos+1][ypos] == 0) {
            feld [xpos+1][ypos] = 153;
            update();
        }

    }
}

void Feld::done ()
{
    if (moving)
        return;

    emitStatus();

    if (checkDone())
        emit gameOver(moves);

}

void Feld::startAnimation (Direction d)
{
    // if animation is already started, return
    if (moving || !chosen)
        return;

    switch (d) {
        case MoveUp:
            if (ypos == 0 || feld [xpos] [ypos-1] != 150)
                return;
            break;
        case MoveDown:
            if (ypos == FIELD_SIZE-1 || feld [xpos] [ypos+1] != 152)
                return;
            break;
        case MoveLeft:
            if (xpos == 0 || feld [xpos-1] [ypos] != 151)
                return;
            break;
        case MoveRight:
            if (xpos == FIELD_SIZE-1 || feld [xpos+1] [ypos] != 153)
                return;
            break;
        default:
            break;
    }

    // reset validDirs now so that arrows don't get drawn
    resetValidDirs();
    update();

    int x = 0, y = 0;

    moves++;
    emit sendMoves(moves);
    dir = d;

    switch (dir) {
        case MoveUp :
            for (x = xpos, y = ypos-1, anz = 0; y >= 0 && feld [x] [y] == 0; anz++, y--);
            if (anz != 0)
            {
                feld [x] [++y] = feld [xpos] [ypos];
            }
            break;
        case MoveDown :
            for (x = xpos, y = ypos+1, anz = 0; y <= FIELD_SIZE-1 && feld [x] [y] == 0; anz++, y++);
            if (anz != 0)
            {
                feld [x] [--y] = feld [xpos] [ypos];
            }
            break;
        case MoveRight :
            for (x = xpos+1, y = ypos, anz = 0; x <= FIELD_SIZE-1 && feld [x] [y] == 0; anz++, x++);
            if (anz != 0)
            {
                feld [--x] [y] = feld [xpos] [ypos];
            }
            break;
        case MoveLeft :
            for (x = xpos-1, y = ypos, anz = 0; x >= 0 && feld [x] [y] == 0; anz++, x--);
            if (anz != 0)
            {
                feld [++x] [y] = feld [xpos] [ypos];
            }
            break;
        default:
            return;
    }

    if (anz != 0) {
        moving = true;

        // BEGIN: Insert undo information
        uint undoChunk = (undoBegin + undoSize) % MAX_UNDO;
        undo[undoChunk].atom = feld[xpos][ypos];
        undo[undoChunk].oldxpos = xpos;
        undo[undoChunk].oldypos = ypos;
        undo[undoChunk].xpos = x;
        undo[undoChunk].ypos = y;
        undo[undoChunk].dir = dir;
        if (undoSize == MAX_UNDO)
            undoBegin = (undoBegin + 1) % MAX_UNDO;
        else
            ++undoSize;
        redoSize = undoSize;
        emit enableUndo(true);
        emit enableRedo(false);
        // END: Insert undo information

        feld [xpos] [ypos] = 0;

        // absolutkoordinaten des zu verschiebenden bildes
        cx = xpos * m_elemSize;
        cy = ypos * m_elemSize;
        xpos = x;
        ypos = y;
        // m_elemSize animationsstufen
        framesbak = frames = anz * m_elemSize;

        // 10 mal pro sek
        startTimer (10);

        QPainter p(&sprite);
        p.drawPixmap(0, 0, copy, cx, cy, m_elemSize, m_elemSize);
    }

}

void Feld::doUndo ()
{
    if (moving || !chosen || undoSize == 0)
        return;

    UndoInfo &undo_info = undo[(undoBegin + --undoSize) % MAX_UNDO];
    emit enableUndo(undoSize != 0);
    emit enableRedo(true);

    --moves;
    emit sendMoves(moves);

    moving = true;
    resetValidDirs ();

    cx = undo_info.xpos;
    cy = undo_info.ypos;
    xpos = undo_info.oldxpos;
    ypos = undo_info.oldypos;
    feld[cx][cy] = 0;
    feld[xpos][ypos] = undo_info.atom;
    cx *= m_elemSize; cy *= m_elemSize;
    framesbak = frames =
        m_elemSize * (abs (undo_info.xpos - undo_info.oldxpos) +
                abs (undo_info.ypos - undo_info.oldypos) );
    startTimer (10);
    dir = (Direction) -((int) undo_info.dir);
    QPainter p(&sprite);
    p.drawPixmap(0, 0, copy, cx, cy, m_elemSize, m_elemSize);
}

void Feld::doRedo ()
{
    if (moving || !chosen || undoSize == redoSize)
        return;

    UndoInfo &undo_info = undo[(undoBegin + undoSize++) % MAX_UNDO];

    emit enableUndo(true);
    emit enableRedo(undoSize != redoSize);

    ++moves;
    emit sendMoves(moves);

    moving = true;
    resetValidDirs ();

    cx = undo_info.oldxpos;
    cy = undo_info.oldypos;
    xpos = undo_info.xpos;
    ypos = undo_info.ypos;
    feld[cx][cy] = 0;
    feld[xpos][ypos] = undo_info.atom;
    cx *= m_elemSize; cy *= m_elemSize;
    framesbak = frames =
        m_elemSize * (abs (undo_info.xpos - undo_info.oldxpos) +
                abs (undo_info.ypos - undo_info.oldypos) );
    startTimer (10);
    dir = undo_info.dir;
    QPainter p(&sprite);
    p.drawPixmap(0, 0, copy, cx, cy, m_elemSize, m_elemSize);
}

void Feld::mouseMoveEvent (QMouseEvent *e)
{
    // warning: mouseMoveEvents can report positions upto 1 pixel outside
    //          of the field widget, so we must be sure handle this case

    if( e->pos().x() < 0 || e->pos().x() >= 450 ||
            e->pos().y() < 0 || e->pos().y() >= 450 )
    {
        setCursor(Qt::ArrowCursor);
    }
    else
    {
        int x = e->pos ().x () / m_elemSize;
        int y = e->pos ().y () / m_elemSize;

        // verschiedene cursor je nach pos
        if (feld[x][y] != 254 && feld [x] [y] != 0)
            setCursor (Qt::CrossCursor);
        else
            setCursor (Qt::ArrowCursor);
    }
}


bool Feld::checkDone ()
{
    int molecWidth = mol->molecSize().width();
    int molecHeight = mol->molecSize().height();
    int i = 0;
    int j = 0;

    // find first atom in molecule
    uint firstAtom = 0;
    for(j = 0; j < molecHeight && !firstAtom; ++j)
        firstAtom = mol->getAtom(0, j);

    // wot no atom?
    if(!firstAtom)
        return true; // true skips to next level

    // position of first atom (in molecule coordinates)
    int mx = 0;
    int my = j - 1;

    QRect extent(0, 0, FIELD_SIZE - molecWidth + 1, FIELD_SIZE - molecHeight + 1);
    extent.translate(0, my);

    // find first atom in playing field
    for(i = extent.left(); i <= extent.right(); ++i)
    {
        for(j = extent.top(); j <= extent.bottom(); ++j)
        {
            if(feld[i][j] == firstAtom)
            {
                // attempt to match playing field to molecule
                int ox = i - mx; // molecule origin (in field coordinates)
                int oy = j - my; // molecule origin (in field coordinates)
                ++my; // no need to test first atom again
                while(mx < molecWidth)
                {
                    while(my < molecHeight)
                    {
                        uint nextAtom = mol->getAtom(mx, my);
                        if(nextAtom != 0 && feld[ox + mx][oy + my] != nextAtom)
                            return false;
                        ++my;
                    }
                    my = 0;
                    ++mx;
                }
                return true;
            }
        }
    }
    // if we got here, then the first atom is too low or too far right
    // for the molecule to be assembled correctly
    return false;
}


void Feld::timerEvent (QTimerEvent *)
{
    // animation beenden
    if (frames <= 0)
    {
        moving = false;
        QAbstractEventDispatcher::instance()->unregisterTimers(this);
        done();
        dir = None;
    }
    else
    {
        frames -= settings.anim_speed;
        if (frames < 0)
            frames = 0;

        update();
    }
}

void Feld::paintMovingAtom(QPainter &paint)
{
    int a = settings.anim_speed;

    switch(dir)
    {
        case MoveUp:
            paint.drawPixmap(cx, cy - framesbak + frames, sprite);
            if(framesbak - frames > 0)
                paint.fillRect(cx, cy - framesbak + frames + m_elemSize, m_elemSize, a, Qt::black);
            break;
        case MoveDown:
            paint.drawPixmap(cx, cy + framesbak - frames, sprite);
            if(framesbak - frames > 0)
                paint.fillRect(cx, cy + framesbak - frames - a, m_elemSize, a, Qt::black);
            break;
        case MoveRight:
            paint.drawPixmap(cx + framesbak - frames, cy, sprite);
            if(framesbak - frames > 0)
                paint.fillRect(cx + framesbak - frames - a, cy, a, m_elemSize, Qt::black);
            break;
        case MoveLeft:
            paint.drawPixmap(cx - framesbak + frames, cy, sprite);
            if(framesbak - frames > 0)
                paint.fillRect(cx - framesbak + frames + m_elemSize, cy, a, m_elemSize, Qt::black);
            break;
        case None:
            break;
    }
}

void Feld::resizeEvent( QResizeEvent *ev)
{
    copy = QPixmap(ev->size());
    copy.fill(Qt::black);
    m_elemSize = qMin( ev->size().width(), ev->size().height() ) / FIELD_SIZE;
    m_renderer->setElementSize( m_elemSize );
    update();
}

void Feld::paintEvent( QPaintEvent * )
{
    int i, j, x, y;

    QPainter paint ( &copy );

    if (moving)
    {
        paintMovingAtom(paint);
        QPainter p(this);
        p.drawPixmap(0, 0, copy);
        return;
    }

    paint.fillRect(0, 0, FIELD_SIZE * m_elemSize, FIELD_SIZE * m_elemSize, Qt::black);

    kDebug() << " -= begin paint event =- " << endl;
    for (i = 0; i < FIELD_SIZE; i++)
    {
        for (j = 0; j < FIELD_SIZE; j++)
        {
            if(moving && i == xpos && j == ypos)
                continue;

            x = i * m_elemSize;
            y = j * m_elemSize;

            QPixmap aPix;
            // FIXME dimsuz: move away from all this digits! :)
            // wall
            if (feld[i][j] == 254) {
                aPix = m_renderer->renderNonAtom('#');
            } 
            else if (feld[i][j] == 150) {
                aPix = m_renderer->renderNonAtom('^');
            }
            else if (feld[i][j] == 151) {
                aPix = m_renderer->renderNonAtom('<');
            }
            else if (feld[i][j] == 152) {
                aPix = m_renderer->renderNonAtom('_');
            }
            else if (feld[i][j] == 153) {
                aPix = m_renderer->renderNonAtom('>');
            }
            else if( getAtom(feld[i][j]).obj != 0 )
            {
                aPix = m_renderer->renderAtom( getAtom(feld[i][j]) );
            }
            paint.drawPixmap(x,y, aPix);
        }
    }
    kDebug() << "-= end paint event =-" << endl;
    QPainter p(this);
    p.drawPixmap(0, 0, copy);
}

#include "feld.moc"
