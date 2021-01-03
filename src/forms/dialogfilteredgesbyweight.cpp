/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 2.8
 Written in Qt
 
                         dialogfilteredgesbyweight.cpp  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/


#include "dialogfilteredgesbyweight.h"
#include <QPushButton>
#include <QDebug>

DialogFilterEdgesByWeight::DialogFilterEdgesByWeight (QWidget *parent) : QDialog (parent)
{
	ui.setupUi(this);	
	connect ( ui.buttonBox,SIGNAL(accepted()), this, SLOT(getUserChoices()) );
	
	(ui.buttonBox) -> button (QDialogButtonBox::Ok) -> setDefault(true);
	
	(ui.overThresholdBt)-> setChecked(true);
		
} 




void DialogFilterEdgesByWeight::getUserChoices(){
	qDebug()<< "Dialog: gathering Data!...";
	bool overThreshold=false;
	float my_threshold = static_cast <float> ( (ui.weightThreshold)->value() );
	if ( ui.overThresholdBt -> isChecked() ) {
		qDebug()<< "Dialog: We will filter edges weighted more than threshold: " << my_threshold;
		overThreshold = true;
	}
	else {
		qDebug()<< "Dialog: We will filter edges weighted less than threshold: " << my_threshold;
		overThreshold = false;
	}	
	qDebug()<< "Dialog: emitting userChoices" ;
	emit userChoices( my_threshold, overThreshold );		
}
