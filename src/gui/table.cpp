/*
 *  The Mana World
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <guichan/graphics.hpp>
#include <guichan/actionlistener.hpp>
#include "table.h"
#include <cassert>



class GuiTableActionListener : public gcn::ActionListener
{
public:
    GuiTableActionListener(GuiTable *_table, gcn::Widget *_widget, int _row, int _column);

    virtual ~GuiTableActionListener(void);

    virtual void action(const gcn::ActionEvent& actionEvent);

protected:
    GuiTable *mTable;
    int mRow;
    int mColumn;
    gcn::Widget *mWidget;
};


GuiTableActionListener::GuiTableActionListener(GuiTable *table, gcn::Widget *widget, int row, int column) :
    mTable(table),
    mRow(row),
    mColumn(column),
    mWidget(widget)
{
    if (widget) {
        widget->addActionListener(this);
        widget->_setParent(table);
    }
}

GuiTableActionListener::~GuiTableActionListener(void)
{
    if (mWidget) {
        mWidget->removeActionListener(this);
        mWidget->_setParent(NULL);
    }
}

void
GuiTableActionListener::action(const gcn::ActionEvent& actionEvent)
{
    mTable->setSelected(mRow, mColumn);
    mTable->distributeActionEvent();
}


GuiTable::GuiTable(TableModel *initial_model) :
    mLinewiseMode(false),
    mModel(NULL),
    mSelectedRow(0),
    mSelectedColumn(0),
    mTopWidget(NULL)
{
    setModel(initial_model);
    addMouseListener(this);
    addKeyListener(this);
}

GuiTable::~GuiTable(void)
{
    delete mModel;
}

TableModel *
GuiTable::getModel(void) const
{
    return mModel;
}

void
GuiTable::setModel(TableModel *new_model)
{
    if (mModel) {
        uninstallActionListeners();
        mModel->removeListener(this);
    }

    mModel = new_model;
    installActionListeners();

    new_model->installListener(this);
    recomputeDimensions();
}

void
GuiTable::recomputeDimensions(void)
{
    int rows_nr = mModel->getRows();
    int columns_nr = mModel->getColumns();
    int width = 0;
    int height = 0;

    if (mSelectedRow >= rows_nr)
        mSelectedRow = rows_nr - 1;

    if (mSelectedColumn >= columns_nr)
        mSelectedColumn = columns_nr - 1;

    for (int i = 0; i < columns_nr; i++)
        width += getColumnWidth(i);

    height = getRowHeight() * rows_nr;

    setWidth(width);
    setHeight(height);
}

void
GuiTable::setSelected(int row, int column)
{
    mSelectedColumn = column;
    mSelectedRow = row;
}

int
GuiTable::getSelectedRow(void)
{
    return mSelectedRow;
}

int
GuiTable::getSelectedColumn(void)
{
    return mSelectedColumn;
}

void
GuiTable::setLinewiseSelection(bool linewise)
{
    mLinewiseMode = linewise;
}

int
GuiTable::getRowHeight(void)
{
    if (mModel)
        return mModel->getRowHeight() + 1; // border
    else
        return 0;
}

int
GuiTable::getColumnWidth(int i)
{
    if (mModel)
        return mModel->getColumnWidth(i) + 1; // border
    else
        return 0;
}

void
GuiTable::uninstallActionListeners(void)
{
    for (std::vector<GuiTableActionListener *>::const_iterator it = action_listeners.begin(); it != action_listeners.end(); it++)
        delete *it;
    action_listeners.clear();
}

void
GuiTable::installActionListeners(void)
{
    int rows = mModel->getRows();
    int columns = mModel->getColumns();

    for (int row = 0; row < rows; ++row)
        for (int column = 0; column < columns; ++column) {
            gcn::Widget *widget = mModel->getElementAt(row, column);
            action_listeners.push_back(new GuiTableActionListener(this, widget,
                                                                        row, column));
        }

    _setFocusHandler(_getFocusHandler()); // propagate focus handler to widgets
}

// -- widget ops
void
GuiTable::draw(gcn::Graphics* graphics)
{
    graphics->setColor(getBackgroundColor());
    graphics->fillRectangle(gcn::Rectangle(0, 0, getWidth(), getHeight()));

    if (!mModel)
        return;

    // First, determine how many rows we need to draw, and where we should start.
    int first_row = -(getY() / getRowHeight());

    if (first_row < 0)
        first_row = 0;

    int rows_nr = 1 + (getHeight() / getRowHeight()); // May overestimate by one.

    int max_rows_nr = mModel->getRows() - first_row; // clip if neccessary:
    if (max_rows_nr < rows_nr)
        rows_nr = max_rows_nr;
 
    // Now determine the first and last column
    // Take the easy way out; these are usually bounded and all visible.
    int first_column = 0;
    int last_column = mModel->getColumns() - 1;

    // Set up everything for drawing
    int height = getRowHeight();
    int y_offset = first_row * height;

    for (int r = first_row; r < first_row + rows_nr; ++r) {
        int x_offset = 0;

        for (int c = first_column; c <= last_column; ++c) {
            gcn::Widget *widget = mModel->getElementAt(r, c);
            int width = getColumnWidth(c);
            if (widget) {
                gcn::Rectangle bounds(x_offset, y_offset, width, height);

                if (widget == mTopWidget) {
                    bounds.height = widget->getHeight();
                    bounds.width = widget->getWidth();
                }

                widget->setDimension(bounds);

                graphics->pushClipArea(bounds);
                widget->draw(graphics);
                graphics->popClipArea();

                if (!mLinewiseMode
                    && c == mSelectedColumn
                    && r == mSelectedRow)
                    graphics->drawRectangle(bounds);
            }

            x_offset += width;
        }

        if (mLinewiseMode
            && r == mSelectedRow)
            graphics->drawRectangle(gcn::Rectangle(0, y_offset,
                                                   x_offset, height));

        y_offset += height;
    }

    if (mTopWidget) {
        gcn::Rectangle bounds = mTopWidget->getDimension();
        graphics->pushClipArea(bounds);
        mTopWidget->draw(graphics);
        graphics->popClipArea();
    }
}

void
GuiTable::logic(void)
{
}

void
GuiTable::moveToTop(gcn::Widget *widget)
{
    gcn::Widget::moveToTop(widget);
    this->mTopWidget = widget;
}

void
GuiTable::moveToBottom(gcn::Widget *widget)
{
    gcn::Widget::moveToBottom(widget);
    if (widget == this->mTopWidget)
        this->mTopWidget = NULL;
}

gcn::Rectangle
GuiTable::getChildrenArea(void)
{
    return gcn::Rectangle(0, 0, getWidth(), getHeight());
}

// -- KeyListener notifications
void
GuiTable::keyPressed(gcn::KeyEvent& keyEvent)
{
}


// -- MouseListener notifications
void
GuiTable::mousePressed(gcn::MouseEvent& mouseEvent)
{
    if (mouseEvent.getButton() == gcn::MouseEvent::LEFT) {
        int row = getRowForY(mouseEvent.getY());
        int column = getColumnForX(mouseEvent.getX());

        if (row > -1 && column > -1) {
            mSelectedColumn = column;
            mSelectedRow = row;
        }

        distributeActionEvent();
    }
}

void
GuiTable::mouseWheelMovedUp(gcn::MouseEvent& mouseEvent)
{
}

void
GuiTable::mouseWheelMovedDown(gcn::MouseEvent& mouseEvent)
{
}
        
void
GuiTable::mouseDragged(gcn::MouseEvent& mouseEvent)
{
}

// -- TableModelListener notifications
void
GuiTable::modelUpdated(bool completed)
{
    if (completed) {
        recomputeDimensions();
        installActionListeners();
    } else // before the update?
        uninstallActionListeners();
}

gcn::Widget *
GuiTable::getWidgetAt(int x, int y)
{
    int row = getRowForY(y);
    int column = getColumnForX(x);

    if (mTopWidget
        && mTopWidget->getDimension().isPointInRect(x, y))
        return mTopWidget;

    if (row > -1
        && column > -1) {
        gcn::Widget *w = mModel->getElementAt(row, column);
        if (w->isFocusable())
            return w;
        else
            return NULL; // Grab the event locally
    } else
        return NULL;
}

int
GuiTable::getRowForY(int y)
{
   int row = y / getRowHeight();

   if (row < 0
       || row >= mModel->getRows())
       return -1;
   else
       return row;
}

int
GuiTable::getColumnForX(int x)
{
    int column;
    int delta = 0;

    for (column = 0; column < mModel->getColumns(); column++) {
        delta += getColumnWidth(column);
        if (x <= delta)
            break;
    }

    if (column < 0
        || column >= mModel->getColumns())
        return -1;
    else
        return column;
}


void
GuiTable::_setFocusHandler(gcn::FocusHandler* focusHandler)
{
    gcn::Widget::_setFocusHandler(focusHandler);

    if (mModel)
        for (int r = 0; r < mModel->getRows(); ++r)
            for (int c = 0; c < mModel->getColumns(); ++c) {
                gcn::Widget *w = mModel->getElementAt(r, c);
                if (w)
                    w->_setFocusHandler(focusHandler);
            }
}