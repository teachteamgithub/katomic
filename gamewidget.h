/*

  Copyright (C) 1998   Andreas Wüst (AndreasWuest@gmx.de)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  */
#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

class Feld;
class Molek;
class QScrollBar;
class QLabel;
class KScoreDialog;

#include <QWidget>

class GameWidget : public QWidget
{
    Q_OBJECT

 public:

    GameWidget ( QWidget *parent );

    ~GameWidget();

 signals:
    void enableRedo(bool enable);
    void enableUndo(bool enable);

 public slots:
    // bringt level auf neuesten stand
    void updateLevel (int);

    // restart current level
    void restartLevel();

    // getbutton erhält button der gedrückt wurde
    void getButton (int);

    void gameOver(int moves);

    // use this slot to update the moves continually
    void getMoves(int moves);

    // Menupunkt Highscores im Pop-up Menu, der Highscore anzeigt
    void showHighscores ();

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void nextAtom();
    void previousAtom();
    void doUndo ();
    void doRedo ();

 protected:

    // stellt das spielfeld dar !
    Feld *feld;

    // stellt molekül dar
    Molek *molek;

    // scorllbar zur levelwahl
    QScrollBar *scrl;

    // important labels : highest and current scores
    QLabel *hs, *ys;
    QString highest, current;

    int nlevels;

    KScoreDialog *highScore;
};

#endif
