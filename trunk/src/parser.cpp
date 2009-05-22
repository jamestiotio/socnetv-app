/***************************************************************************
 SocNetV: Social Networks Visualizer 
 version: 0.6
 Written in Qt 4.4
 
                         parser.cpp  -  description
                             -------------------
    copyright            : (C) 2005-2009 by Dimitris B. Kalamaras
    email                : dimitris.kalamaras@gmail.com
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

#include "parser.h"

#include <QFile>
#include <QTextStream>
#include <QString>
#include <QRegExp>
#include <QStringList>
#include <QtDebug>		//used for qDebug messages
#include <QPointF>
#include <QMessageBox>
#include <list>
#include "graph.h"	//needed for setParent

void Parser::load(QString fn, int iNS, QString iNC, QString iLC, QString iNSh, bool iSL, int width, int height){
	qDebug("Parser: load()");
	initNodeSize=iNS;
	initNodeColor=iNC;
	initEdgeColor=iLC;
	initNodeShape=iNSh;
	initShowLabels=iSL;;
	undirected=FALSE; arrows=FALSE; bezier=FALSE;
	networkName="";
	fileName=fn;
	gwWidth=width;
	gwHeight=height;
	randX=0;
	randY=0;
	qDebug("Parser: calling start() to start a new QThread!");
	if (!isRunning()) 
		start(QThread::NormalPriority);
}



/**
	Tries to load a file as DL-formatted network. If not it returns -1
*/
int Parser::loadDL(){
	qDebug ("Parser: loadDL");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );

	QString str, label;
	
	int i=0, j=0, lineCounter=0, mark=0, nodeNum=0;
	edgeWeight=0;
	bool labels_flag=false, data_flag=false, intOK=false, floatOK=false;
	QStringList lineElement;
	networkName="";
	totalLinks=0;

	while ( !ts.atEnd() )   {
		str= ts.readLine();
		lineCounter++;
		if ( str.startsWith("%") || str.startsWith("#")  || str.isEmpty() ) continue;  //neglect comments
	
		if ( (lineCounter == 1) &&  (!str.startsWith("DL",Qt::CaseInsensitive)  ) ) {  
			qDebug("*** Parser-loadDL(): not a DL file. Aborting!");
			file.close();
			return -1;
		}

		if (str.startsWith( "N=", Qt::CaseInsensitive) ||  str.startsWith( "N =", Qt::CaseInsensitive) )  {   
			mark=str.indexOf("=");
			str=str.right(str.size()-mark-1);
			qDebug()<< "N = : " << str.toAscii() ;
			aNodes=str.toInt(&intOK,10);   
			if (!intOK) { qDebug()<< "Parser: loadDL(): conversion error..." ; return -1;}
		}

		if (str.startsWith( "FORMAT =", Qt::CaseInsensitive) || str.startsWith( "FORMAT=", Qt::CaseInsensitive))  {   
			mark=str.indexOf("=");
			str=str.right(str.size()-mark-1);
			qDebug()<<  "FORMAT = : " <<  str.toAscii() ;
		}

		if (str.startsWith( "labels", Qt::CaseInsensitive) ) {
		 	labels_flag=true; data_flag=false;
			continue;
		}
		else if ( str.startsWith( "data:", Qt::CaseInsensitive) || str.startsWith( "data :", Qt::CaseInsensitive) ) {
		 	data_flag=true; labels_flag=false;
			continue;
		}

		if (labels_flag) {  //read a label and create a node in a random position

			label=str;
			randX=rand()%gwWidth;
			randY=rand()%gwHeight;
			nodeNum++;
			qDebug()<<"Creating node at "<< randX<<","<< randY;
			emit createNode(nodeNum, initNodeSize, initNodeColor, label, initNodeColor, QPointF(randX, randY), initNodeShape, initShowLabels);
		
		}
		if ( data_flag){		//read edges
			//SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) 
			lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);
			j=0;
			for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
				qDebug()<< (*it1).toAscii() ;
				if ( (*it1)!="0"){ //here is an non-zero edge weight...
					qDebug()<<  "Parser-loadDL(): there is an edge here";
					edgeWeight=(*it1).toFloat(&floatOK); 
					undirected=false;
					arrows=true;
					bezier=false;
					emit createEdge(i+1, j+1, edgeWeight, initEdgeColor, undirected, arrows, bezier);
					totalLinks++;
					qDebug()<<  "Link from Node i= " <<  i+1 << " to j= "<< j+1;
					qDebug() << "TotalLinks= " << totalLinks;

				}
				j++;
			}
			i++;
		

		}
	}
	//sanity check
	if (nodeNum != aNodes) { 
		qDebug()<< "Error: aborting";
		return -1;
	}
	emit fileType(5, networkName, aNodes, totalLinks, undirected);
	qDebug() << "Parser-loadDL()";
	return 1;

}

/**
	Tries to load the file as Pajek-formatted network. If not it returns -1
*/
int Parser::loadPajek(){
	qDebug ("Parser: loadPajek");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	QString str, label, temp;
	nodeColor="";
	edgeColor="";
	nodeShape="";
	QStringList lineElement;
	bool ok=FALSE, intOk=FALSE, check1=FALSE, check2=FALSE;
	bool nodes_flag=FALSE, edges_flag=FALSE, arcs_flag=FALSE, arcslist_flag=FALSE, matrix_flag=FALSE;
	bool fileContainsNodeColors=FALSE, fileContainsNodesCoords=FALSE;
	bool fileContainsLinksColors=FALSE;
	bool zero_flag=FALSE;
	int  lineCounter=0, i=0, j=0, miss=0, source= -1, target=-1, nodeNum, colorIndex=-1, coordIndex=-1;
	float weight=1;
	list<int> listDummiesPajek;
	networkName="noname";
	totalLinks=0;
	aNodes=0;
	j=0;  //counts how many real nodes exist in the file 
	miss=0; //counts missing nodeNumbers. 
	//if j + miss < nodeNum, it creates (nodeNum-miss) dummy nodes which are deleted in the end.
	QList <int> toBeDeleted;
	while ( !ts.atEnd() )   {
		str= ts.readLine();
		lineCounter++;
		if ( str.startsWith("%") || str.isEmpty() ) continue;

		if (!edges_flag && !arcs_flag && !nodes_flag && !arcslist_flag && !matrix_flag) {
			qDebug("Parser-loadPajek(): reading headlines");
			if ( (lineCounter == 1) &&  (!str.contains("network",Qt::CaseInsensitive) && !str.contains("vertices",Qt::CaseInsensitive) ) ) {  
				//this is not a pajek file. Abort
				qDebug("*** Parser-loadPajek(): Not a Pajek file. Aborting!");
				file.close();
				return -1;
			}
   			else if (str.contains( "network",Qt::CaseInsensitive) )  { //NETWORK NAME
				if (str.contains(" ")) {
					lineElement=str.split(QRegExp("\\s+"));	//split at one or more spaces
					qDebug()<<"Parser-loadPajek(): possible net name: "<<lineElement[1];
					if (lineElement[1]!="") 
						networkName=lineElement[1];
				}
				else 
					networkName = "Unknown";
				qDebug()<<"Parser-loadPajek(): network name: "<<networkName;
				continue;
			}
			if (str.contains( "vertices", Qt::CaseInsensitive) )  {   
				lineElement=str.split(QRegExp("\\s+"));
				if (!lineElement[1].isEmpty()) 	aNodes=lineElement[1].toInt(&intOk,10);   
				qDebug ("Parser-loadPajek(): Vertices %i.",aNodes);
				continue;
			}
			qDebug("Parser-loadPajek(): headlines end here");
		}
		/**SPLIT EACH LINE (ON EMPTY SPACE CHARACTERS) IN SEVERAL ELEMENTS*/
		lineElement=str.split(QRegExp("\\s+"), QString::SkipEmptyParts);

		if ( str.contains( "*edges", Qt::CaseInsensitive) ) {
		 	edges_flag=true; arcs_flag=false; arcslist_flag=false; matrix_flag=false;
			continue;
		}
		else if ( str.contains( "*arcs", Qt::CaseInsensitive) ) { 
			arcs_flag=true; edges_flag=false; arcslist_flag=false; matrix_flag=false;
			continue;
		}
		else if ( str.contains( "*arcslist", Qt::CaseInsensitive) ) { 
			arcs_flag=false; edges_flag=false; arcslist_flag=true; matrix_flag=false;
			continue;
		}
		else if ( str.contains( "*matrix", Qt::CaseInsensitive) ) { 
			arcs_flag=false; edges_flag=false; arcslist_flag=false; matrix_flag=true;
			continue;
		}

		/** READING NODES */
		if (!edges_flag && !arcs_flag && !arcslist_flag && !matrix_flag) {
			qDebug("=== Reading nodes ===");
			nodes_flag=TRUE;
			nodeNum=lineElement[0].toInt(&intOk, 10);
			qDebug()<<"nodeNum "<<nodeNum;
			if (nodeNum==0) {
				qDebug ("Node is zero numbered! Raising zero-start-flag - increasing nodenum");
				zero_flag=TRUE;
			}
			if (zero_flag){
				nodeNum++;
			}
			if (lineElement.size() < 2 ){
				label=lineElement[0];
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				nodeColor=initNodeColor;
				nodeShape=initNodeShape;
			}
			else {	/** NODELABEL */
				label=lineElement[1];
				qDebug()<< "node label: " << lineElement[1].toAscii();
				str.remove (0, str.lastIndexOf(label)+label.size() );	
				qDebug()<<"cropped str: "<< str.toAscii();
				if (label.contains('"', Qt::CaseInsensitive) )
					label=label.remove('\"');
				qDebug()<<"node label now: " << label.toAscii();
								
				/** NODESHAPE: There are four possible . */
				if (str.contains("Ellipse", Qt::CaseInsensitive) ) nodeShape="ellipse";
				else if (str.contains("circle", Qt::CaseInsensitive) ) nodeShape="circle";
				else if (str.contains("box", Qt::CaseInsensitive) ) nodeShape="box";
				else if (str.contains("triangle", Qt::CaseInsensitive) ) nodeShape="triangle";
				else nodeShape="diamond";
				/** NODECOLORS */
				//if there is an "ic" tag, a specific NodeColor for this node follows...
				if (str.contains("ic",Qt::CaseInsensitive)) { 
					for (register int c=0; c< lineElement.count(); c++) {
						if (lineElement[c] == "ic") { 
							//the colourname is at c+1 position.
							nodeColor=lineElement[c+1];
							
							fileContainsNodeColors=TRUE;
							break;
						}
					}
					qDebug()<<"nodeColor:" << nodeColor;
					if (nodeColor.contains (".") )  nodeColor=initNodeColor;
				}
				else { //there is no nodeColor. Use the default
					qDebug("No nodeColor");
					fileContainsNodeColors=FALSE;
					nodeColor=initNodeColor;
					
				}
				/**READ NODE COORDINATES */
				if ( str.contains(".",Qt::CaseInsensitive) ) { 
					for (register int c=0; c< lineElement.count(); c++)   {
						temp=lineElement.at(c);
						qDebug()<< temp.toAscii();
						if ((coordIndex=temp.indexOf(".", Qt::CaseInsensitive)) != -1 ){ 	
							if (lineElement.at(c-1) == "ic" ) continue;  //pajek declares colors with numbers!
							if ( !temp[coordIndex-1].isDigit()) continue;  //needs 0.XX
							if (c+1 == lineElement.count() ) {//first coord zero, i.e: 0  0.455
								qDebug ()<<"coords: " <<lineElement.at(c-1).toAscii() << " " <<temp.toAscii() ;
								randX=lineElement.at(c-1).toDouble(&check1);
								randY=temp.toDouble(&check2);
							}
							else {
								qDebug ()<<"coords: " << temp.toAscii() << " " <<lineElement[c+1].toAscii();
								randX=temp.toDouble(&check1);
								randY=lineElement[c+1].toDouble(&check2);
							}

							if (check1 && check2)    {
								randX=randX * gwWidth;
								randY=randY * gwHeight;
								fileContainsNodesCoords=TRUE;
							}
							if (randX <= 0.0 || randY <= 0.0 ) {
								randX=rand()%gwWidth;
								randY=rand()%gwHeight;
							}
							break;
						}
					}
					qDebug()<<"Coords: "<<randX << randY<< gwHeight;
				}
				else { 
					fileContainsNodesCoords=FALSE;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<"No coords. Using random "<<randX << randY;
				}
			}
			/**START NODE CREATION */
			qDebug ()<<"Creating node numbered "<< nodeNum << " Real nodes count (j)= "<< j+1;
			j++;  //Controls the real number of nodes.
			//If the file misses some nodenumbers then we create dummies and delete them afterwards!
			if ( j + miss < nodeNum)  {
				qDebug ()<<"MW There are "<< j << " nodes but this node has number "<< nodeNum;
				for (int num=j; num< nodeNum; num++) {
					qDebug()<< "Parser-loadPajek(): Creating dummy node number num = "<< num;
					qDebug()<<"Creating node at "<< randX<<","<< randY;
					emit createNode(num,initNodeSize, nodeColor, label, lineElement[3], QPointF(randX, randY), nodeShape, initShowLabels);
					listDummiesPajek.push_back(num);  
					miss++;
				}
			}
			else if ( j > nodeNum ) {
				qDebug ("Error: This Pajek net declares this node with nodeNumber smaller than previous nodes. Aborting");
				return -1;	
			}
			qDebug()<<"Creating node at "<< randX<<","<< randY;
			emit createNode(nodeNum,initNodeSize, nodeColor, label, nodeColor, QPointF(randX, randY), nodeShape, initShowLabels);
			initNodeColor=nodeColor; 
		} 	
		/**NODES CREATED. CREATE EDGES/ARCS NOW. */		
		else {
			if (j && j!=aNodes)  {  //if there were more or less nodes than the file declared
				qDebug()<<"*** WARNING ***: The Pajek file declares " << aNodes <<"  nodes, but I found " <<  j << " nodes...." ;
				aNodes=j;
			}
			else if (j==0) {  //if there were no nodes at all, we need to create them now.
				qDebug()<< "The Pajek file declares "<< aNodes<< " but I didnt found any nodes. I will create them....";
				for (int num=j+1; num<= aNodes; num++) {
					qDebug() << "Parser-loadPajek(): Creating node number i = "<< num;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					emit createNode(num,initNodeSize, initNodeColor, QString::number(i), "black", QPointF(randX, randY), initNodeShape, initShowLabels);
				}
				j=aNodes;
			}
			if (edges_flag && !arcs_flag)   {  /**EDGES */
				qDebug("Parser-loadPajek(): ==== Reading edges ====");
				qDebug()<<lineElement;
				source =  lineElement[0].toInt(&ok, 10);
				target = lineElement[1].toInt(&ok,10);
				if (source == 0 || target == 0 ) return -1;  //  i --> (i-1)   internally
				else if (source < 0 && target >0  ) {  //weights come first...
					edgeWeight  = lineElement[0].toFloat(&ok);
					source=  lineElement[1].toInt(&ok, 10);
					if (lineElement.count()>2) {
						target = lineElement[2].toInt(&ok,10);
					}
					else {
						target = lineElement[1].toInt(&ok,10);  //self link
					}
				}
				else if (lineElement.count()>2)
					edgeWeight =lineElement[2].toFloat(&ok);
				else 
					edgeWeight =1.0;

				qDebug()<<"Parser-loadPajek(): weight "<< weight;

				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					fileContainsLinksColors=TRUE;
					colorIndex=lineElement.indexOf( QRegExp("[c]"), 0 )  +1;
					if (colorIndex >= lineElement.count()) edgeColor=initEdgeColor;
					else 	edgeColor=lineElement [ colorIndex ];
					if (edgeColor.contains (".") )  edgeColor=initEdgeColor;
					qDebug()<< " current color "<< edgeColor;
 				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					edgeColor=initEdgeColor;
				}
				undirected=true;
				arrows=true;
				bezier=false;
				qDebug()<< "Parser-loadPajek(): Create edge between " << source << " - "<< target;
				emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
				totalLinks=totalLinks+2;

			} //end if EDGES 
			else if (!edges_flag && arcs_flag)   {  /** ARCS */
				qDebug("Parser-loadPajek(): === Reading arcs ===");
				source=  lineElement[0].toInt(&ok, 10);
				target = lineElement[1].toInt(&ok,10);
				if (source == 0 || target == 0 ) return -1;   //  i --> (i-1)   internally
				else if (source < 0 && target >0 ) {  //weights come first...
					edgeWeight  = lineElement[0].toFloat(&ok);
					source=  lineElement[1].toInt(&ok, 10);
					if (lineElement.count()>2) {
						target = lineElement[2].toInt(&ok,10);
					}
					else {
						target = lineElement[1].toInt(&ok,10);  //self link
					}
				}
				else if (lineElement.count()>2)
					edgeWeight  =lineElement[2].toFloat(&ok);
				else 
					edgeWeight =1.0;

				if (lineElement.contains("c", Qt::CaseSensitive ) ) {
					qDebug("Parser-loadPajek(): file with link colours");
					edgeColor=lineElement.at ( lineElement.indexOf( QRegExp("[c]"), 0 ) + 1 );
					fileContainsLinksColors=TRUE;
				}
				else  {
					qDebug("Parser-loadPajek(): file with no link colours");
					edgeColor=initEdgeColor;
				}
				undirected=false;
				arrows=true;
				bezier=false;
				qDebug()<<"Parser-loadPajek(): Creating arc from node "<< source << " to node "<< target << " with weight "<< weight;
				emit createEdge(source, target, edgeWeight , edgeColor, undirected, arrows, bezier);
				totalLinks++;
			} //else if ARCS
			else if (arcslist_flag)   {  /** ARCSlist */
				qDebug("Parser-loadPajek(): === Reading arcs list===");
				if (lineElement[0].startsWith("-") ) lineElement[0].remove(0,1);
				source= lineElement[0].toInt(&ok, 10);
				fileContainsLinksColors=FALSE;
				edgeColor=initEdgeColor;
				undirected=false;
				arrows=true;
				bezier=false;
				for (register int index = 1; index < lineElement.size(); index++) {
					target = lineElement.at(index).toInt(&ok,10);					
					qDebug()<<"Parser-loadPajek(): Creating arc source "<< source << " target "<< target << " with weight "<< weight;
					emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
					totalLinks++;
				}
			} //else if ARCSLIST
			else if (matrix_flag)   {  /** matrix */
				qDebug("Parser-loadPajek(): === Reading matrix of edges===");
				i++;
				source= i;
				fileContainsLinksColors=FALSE;
				edgeColor=initEdgeColor;
				undirected=false;
				arrows=true;
				bezier=false;
				for (target = 0; target < lineElement.size(); target ++) {
					if ( lineElement.at(target) != "0" ) {
						edgeWeight  = lineElement.at(target).toFloat(&ok);					
						qDebug()<<"Parser-loadPajek(): Creating arc source "<< source << " target "<< target +1<< " with weight "<< weight;
						emit createEdge(source, target+1, edgeWeight, edgeColor, undirected, arrows, bezier);
						totalLinks++;
					}
				}
			} //else if matrix
		} //end if BOTH ARCS AND EDGES
	} //end WHILE
	file.close();
	if (j==0) return -1;
	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(1, networkName, aNodes, totalLinks, undirected);
	
	qDebug("Parser-loadPajek(): Removing all dummy aNodes, if any");
	if (listDummiesPajek.size() > 0 ) {
		qDebug("Trying to delete the dummies now");
		for ( list<int>::iterator it=listDummiesPajek.begin(); it!=listDummiesPajek.end(); it++ ) {
			emit removeDummyNode(*it);
		}
	}
	qDebug("Parser-loadPajek(): Clearing DumiesList from Pajek");
	listDummiesPajek.clear();
	exit(0);
	return 1;

}






/**
	Tries to load the file as adjacency sociomatrix-formatted. If not it returns -1
*/
int Parser::loadAdjacency(){
	qDebug("Parser: loadAdjacency()");
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	networkName="";
	QString str;
	QStringList lineElement;
	int i=0, j=0,  aNodes=0;
	edgeWeight=1.0;
	bool intOK=FALSE;

	while ( !ts.atEnd() )   {
		str= ts.readLine() ;
		str=str.simplified();  // transforms "/t", "  ", etc to plain " ".
		if (str.isEmpty() ) continue;	
		if ( str.contains("vertices",Qt::CaseInsensitive) || (str.contains("network",Qt::CaseInsensitive) || str.contains("graph",Qt::CaseInsensitive)  || str.contains("digraph",Qt::CaseInsensitive) ||  str.contains("DL",Qt::CaseInsensitive) || str.contains("list",Qt::CaseInsensitive)) || str.contains("graphml",Qt::CaseInsensitive) || str.contains("xml",Qt::CaseInsensitive)  ) {
			qDebug()<< "*** Parser:loadAdjacency(): Not an Adjacency-formatted file. Aborting!!";
			file.close();		
 		 	return -1;    
		}

		lineElement=str.split(" ");
		if (i == 0 ) {
			aNodes=lineElement.count();
			qDebug("Parser-loadAdjacency(): There are %i nodes in this file", aNodes);		
			for (j=0; j<aNodes; j++) {
				qDebug("Parser-loadAdjacency(): Calling createNode() for node %i", j+1);
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				qDebug()<<"Parser-loadAdjacency(): no coords. Using random "<<randX << randY;

	// 			nodeNum,initNodeSize,nodeColor, label, lColor, QPointF(X, Y), nodeShape
				emit createNode( j+1,initNodeSize, 
								initNodeColor, QString::number(j+1), 
								"black", QPointF(randX, randY), 
								initNodeShape, false
								);
			}
		}
		qDebug("Parser-loadAdjacency(): Finished creating new nodes");
		if ( aNodes != (int) lineElement.count() ) return -1;	
		j=0;
		qDebug("Parser-loadAdjacency(): Starting creating links");		
		for (QStringList::Iterator it1 = lineElement.begin(); it1!=lineElement.end(); ++it1)   {
			if ( (*it1)!="0"){
				qDebug("Parser-loadAdjacency(): there is a link here");
				edgeWeight =(*it1).toFloat(&intOK);
				undirected=false;
				arrows=true;
				bezier=false;
				emit createEdge(i+1, j+1, edgeWeight, initEdgeColor, undirected, arrows, bezier);
				totalLinks++;

				qDebug("Link from Node i=%i to j=%i", i+1, j+1);
				qDebug("TotalLinks= %i", totalLinks);
			}

			j++;
		}
		i++;
	}
	file.close();

	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(2, networkName, aNodes, totalLinks, undirected);
	return 1;
}






/**
	Tries to load a file as GraphML (not GML) formatted network. 
	If not it returns -1
*/
int Parser::loadGraphML(){
	qDebug("Parser: loadGraphML()");
	aNodes=0;
	totalLinks=0;
	nodeNumber.clear();
	bool_key=false; bool_node=false; bool_edge=false;
	key_id = "";
	key_name = "";
	key_type = "";
	key_value = "";
	initEdgeWeight = 1;
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;

	QXmlStreamReader *xml = new QXmlStreamReader();
	
	xml->setDevice(&file);
	
	while (!xml->atEnd()) {
		xml->readNext();
		qDebug()<< " loadGraphML(): xml->token "<< xml->tokenString();
		if (xml->isStartElement()) {
			qDebug()<< " loadGraphML(): element name "<< xml->name().toString()<<" version " << xml->attributes().value("version").toString()  ;
			if (xml->name() == "graphml") {	//this is a GraphML document, call method.
				qDebug()<< " loadGraphML(): OK. NamespaceUri is "<< xml->namespaceUri().toString();
				readGraphML(*xml);				
			}
			else {	//not a GraphML doc, return -1.
				xml->raiseError(QObject::tr(" loadGraphML(): The file is not an GraphML version 1.0 file."));
				qDebug()<< "*** loadGraphML(): Error in startElement  ";
 				return -1;
			}
		}
	}
	emit fileType(4, networkName, aNodes, totalLinks, undirected);
	//clear our mess - remove every hash element...
	keyFor.clear();
	keyName.clear();
	keyType.clear(); 
	keyDefaultValue.clear();
	nodeNumber.clear();
	return 1;
}


/*
 * Called from loadGraphML
 * This method checks the xml token name and calls the appropriate function.
 */
void Parser::readGraphML(QXmlStreamReader &xml){
	qDebug()<< " Parser: readGraphML()";
	bool_node=false;
	bool_edge=false;
	bool_key=false;
	Q_ASSERT(xml.isStartElement() && xml.name() == "graph");
	
	while (!xml.atEnd()) { //start reading until QXmlStreamReader end().
		xml.readNext();	//read next token
	
		if (xml.isStartElement()) {	//new token (graph, node, or edge) here
			qDebug()<< "\n  readGraphML(): start of element: "<< xml.name().toString() ;
			if (xml.name() == "graph")	//graph definition token
				readGraphMLElementGraph(xml);
				
			else if (xml.name() == "key")	//key definition token
				readGraphMLElementKey(xml);
								
			else if (xml.name() == "default") //default key value token 
				readGraphMLElementDefaultValue(xml);

			else if (xml.name() == "node")	//graph definition token
				readGraphMLElementNode(xml);
				
			else if (xml.name() == "data")	//data definition token
				readGraphMLElementData(xml);
			
			else if ( xml.name() == "ShapeNode") {
				bool_node =  true;
			}			
			else if (	 ( 
							xml.name() == "Geometry" 
							|| xml.name() == "Fill"
							|| xml.name() == "BorderStyle"
							|| xml.name() == "NodeLabel"
							|| xml.name() == "Shape" 
						)
						&& 	bool_node 
					) {
				readGraphMLElementNodeGraphics(xml);
			}
			
			else if (xml.name() == "edge")	//edge definition token
				readGraphMLElementEdge(xml);
			else if ( xml.name() == "BezierEdge") {
				bool_edge =  true;
			}			

			else if (	 ( 
							xml.name() == "Path"
							|| xml.name() == "LineStyle"
							|| xml.name() == "Arrows"
							|| xml.name() == "EdgeLabel" 
						)
						&& 	bool_edge 
					) {
				readGraphMLElementEdgeGraphics(xml);
			}
			
			else
				readGraphMLElementUnknown(xml);
		}
		
		if (xml.isEndElement()) {		//token ends here
			qDebug()<< "  readGraphML():  element ends here: "<< xml.name().toString() ;
				if (xml.name() == "node")	//node definition end 
					endGraphMLElementNode(xml);
				else if (xml.name() == "edge")	//edge definition end 
					endGraphMLElementEdge(xml);
		}
	}
	
}


// this method reads a graph definition 
// called at Graph element
void Parser::readGraphMLElementGraph(QXmlStreamReader &xml){
	qDebug()<< "   Parser: readGraphMLElementGraph()";
	QString defaultDirection = xml.attributes().value("edgedefault").toString();
	qDebug()<< "    edgedefault "<< defaultDirection;
	if (defaultDirection=="undirected"){
		undirected = true;
	}
	else {
		undirected = false;
	}
	networkName = xml.attributes().value("id").toString();
	qDebug()<< "    graph id  "  << networkName; //store graph id to return it afterwards 
}


// this method reads a key definition 
// called at key element
void Parser::readGraphMLElementKey (QXmlStreamReader &xml){
	
	qDebug()<< "   Parser: readGraphMLElementKey()";
	key_id = xml.attributes().value("id").toString();
 	qDebug()<< "    key id "<< key_id;
	key_what = xml.attributes().value("for").toString();
	keyFor [key_id] = key_what;
	qDebug()<< "    key for "<< key_what;
	
	if (xml.attributes().hasAttribute("attr.name") ) {
		key_name =xml.attributes().value("attr.name").toString();
		keyName [key_id] = key_name;
		qDebug()<< "    key attr.name "<< key_name;		
	}
	if (xml.attributes().hasAttribute("attr.type") ) {
		key_type=xml.attributes().value("attr.type").toString();
		keyType [key_id] = key_type;
		qDebug()<< "    key attr.type "<< key_type;
	}
	else if (xml.attributes().hasAttribute("yfiles.type") ) {
		key_type=xml.attributes().value("yfiles.type").toString();
		keyType [key_id] = key_type;
		qDebug()<< "    key yfiles.type "<< key_type;
	}

}


// this method reads default key values 
// called at a default element (usually nested inside key element)
void Parser::readGraphMLElementDefaultValue(QXmlStreamReader &xml) {
	qDebug()<< "   Parser: readGraphMLElementDefaultValue()";

	key_value=xml.readElementText();
	keyDefaultValue [key_id] = key_value;	//key_id is already stored 
	qDebug()<< "    key default value is "<< key_value;
	if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "    this key default value "<< key_value << " is for nodes color";
			initNodeColor= key_value; 
	}
	if (keyName.value(key_id) == "weight" && keyFor.value(key_id) == "edge" ) {
			qDebug()<< "    this key default value "<< key_value << " is for edges weight";
			conv_OK=false;
			initEdgeWeight= key_value.toFloat(&conv_OK);
			if (!conv_OK) initEdgeWeight = 1;  
	}
	if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
			qDebug()<< "    this key default value "<< key_value << " is for edges color";
			initEdgeColor= key_value; 
	}
}



// this method reads basic node attributes and sets the nodeNumber.
// called at the start of a node element
void Parser::readGraphMLElementNode(QXmlStreamReader &xml){
	node_id = (xml.attributes().value("id")).toString();
	aNodes++;
	qDebug()<<"   Parser: readGraphMLElementNode() node id "<<  node_id << " index " << aNodes << " added to nodeNumber map";

	nodeNumber[node_id]=aNodes;

	//copy default node attribute values. 
	//Some might change when reading element data, some will stay the same...   	
	nodeColor = initNodeColor;
	nodeShape = initNodeShape;
	nodeSize = initNodeSize;
	nodeLabel = node_id;
	bool_node = true;
	randX=rand()%gwWidth;
	randY=rand()%gwHeight;

}


// this method emits the node creation signal.
// called at the end of a node element   
void Parser::endGraphMLElementNode(QXmlStreamReader &xml){
	Q_UNUSED(xml);
	
	qDebug()<<"   Parser: endGraphMLElementNode() *** signal to create node with id "
		<< node_id << " nodenumber "<< aNodes << " coords " << randX << ", " << randY;
	emit createNode(aNodes, nodeSize, nodeColor, nodeLabel, nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
	bool_node = false;
	
}


// this method reads basic edge creation properties.
// called at the start of an edge element
void Parser::readGraphMLElementEdge(QXmlStreamReader &xml){
	qDebug()<< "   Parser: readGraphMLElementEdge() id: " <<	xml.attributes().value("id").toString();
	QString s = xml.attributes().value("source").toString();
	QString t = xml.attributes().value("target").toString();
	if ( (xml.attributes().value("directed")).toString() == "false") 
		undirected = "true";
	source = nodeNumber [s];
	target = nodeNumber [t];
	edgeWeight=initEdgeWeight;
	bool_edge= true;
	qDebug()<< "    edge source "<< s << " num "<< source;
	qDebug()<< "    edge target "<< t << " num "<< target;

	
}


// this method emits the edge creation signal.
// called at the end of edge element   
void Parser::endGraphMLElementEdge(QXmlStreamReader &xml){
	Q_UNUSED(xml);
	qDebug()<<"   Parser: endGraphMLElementEdge() *** emitting signal to create edge from "<< source << " to " << target;
	//FIXME need to return edge label as well!
	emit createEdge(source, target, edgeWeight, edgeColor, undirected, arrows, bezier);
	totalLinks++;
	bool_edge= false;
}


/*
 * this method reads data for edges and nodes 
 * called at a data element (usually nested inside a node an edge element) 
 */ 
void Parser::readGraphMLElementData (QXmlStreamReader &xml){
	key_id = xml.attributes().value("key").toString();
	key_value=xml.text().toString();
	if (key_value.trimmed() == "") 
	{
		qDebug()<< "   Parser: readGraphMLElementData(): text: " << key_value;
		xml.readNext();
		key_value=xml.text().toString();
		qDebug()<< "   Parser: readGraphMLElementData(): text: " << key_value; 
		if (  key_value.trimmed() != "" ) { //if there's simple text after the StartElement,
				qDebug()<< "   Parser: readGraphMLElementData(): key_id " << key_id 
						<< " value is simple text " <<key_value ;
		}
		else {  //no text, probably more tags. Return...
			qDebug()<< "   Parser: readGraphMLElementData(): key_id " << key_id 
							<< " for " <<keyFor.value(key_id) << ". More elements nested here, continuing";
			return;  
		}
		
	}
	
	if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node color: "<< key_value << " for this node";
			nodeColor= key_value; 
	}
	else if (keyName.value(key_id) == "x_coordinate" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node x: "<< key_value << " for this node";
			conv_OK=false;
			randX= key_value.toFloat( &conv_OK ) ;
			if (!conv_OK)
				randX = 0; 
			else 
				randX=randX * gwWidth;
			qDebug()<< "     Using: "<< randX;
	}
	else if (keyName.value(key_id) == "y_coordinate" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node y: "<< key_value << " for this node"; 
			conv_OK=false;
			randY= key_value.toFloat( &conv_OK );
			if (!conv_OK)
				randY = 0;  
			else 
				randY=randY * gwHeight;	
			qDebug()<< "     Using: "<< randY;
	}
	else if (keyName.value(key_id) == "shape" && keyFor.value(key_id) == "node" ) {
			qDebug()<< "     Data found. Node shape: "<< key_value << " for this node";
			nodeShape= key_value; 
	}	
	else if (keyName.value(key_id) == "color" && keyFor.value(key_id) == "edge" ) {
			qDebug()<< "     Data found. Edge color: "<< key_value << " for this edge";
			edgeColor= key_value; 
	}
	else if ( ( keyName.value(key_id) == "value" ||  keyName.value(key_id) == "weight" ) && keyFor.value(key_id) == "edge" ) {
			conv_OK=false;
			edgeWeight= key_value.toFloat( &conv_OK );
			if (!conv_OK) 
				edgeWeight = 1.0;  	
 			qDebug()<< "     Data found. Edge value: "<< key_value << " Using "<< edgeWeight << " for this edge";       
	}
	else if ( keyName.value(key_id) == "size of arrow"  && keyFor.value(key_id) == "edge" ) {
			conv_OK=false;
			float temp = key_value.toFloat( &conv_OK );
			if (!conv_OK) arrowSize = 1;
			else  arrowSize = temp;
			qDebug()<< "     Data found. Edge arrow size: "<< key_value << " Using  "<< arrowSize << " for this edge";
	}


	
}



/**
 * 	Reads node graphics data and properties: label, color, shape, size, coordinates, etc.
 */
void Parser::readGraphMLElementNodeGraphics(QXmlStreamReader &xml) {
	qDebug()<< "       Parser: readGraphMLElementNodeGraphics(): element name "<< xml.name();
	float tempX =-1, tempY=-1, temp=-1;
	QString color;
	//qDebug()<< " loadGraphML(): element qualified name "<< xml.qualifiedName().toString();
	if ( xml.name() == "Geometry" ) {

			if ( xml.attributes().hasAttribute("x") ) {
				conv_OK=false;
				tempX = xml.attributes().value("x").toString().toFloat (&conv_OK) ;
				if (conv_OK) 
					randX = tempX;	
			}
			if ( xml.attributes().hasAttribute("y") ) {
				conv_OK=false;
				tempY = xml.attributes().value("y").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					randY = tempY;
			}
			qDebug()<< "        Node Coordinates: " << tempX << " " << tempY << " Using coordinates" << randX<< " "<<randY;
			if (xml.attributes().hasAttribute("width") ) {
				conv_OK=false;
				temp = xml.attributes().value("width").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					nodeSize = temp;
				qDebug()<< "        Node Size: " << temp<< " Using nodesize" << nodeSize;
			}
			if (xml.attributes().hasAttribute("shape") ) {
				nodeShape = xml.attributes().value("shape").toString();
				qDebug()<< "        Node Shape: " << nodeShape;
			}

	}
	else if (xml.name() == "Fill" ){
		if ( xml.attributes().hasAttribute("color") ) {
			nodeColor= xml.attributes().value("color").toString();	
			qDebug()<< "        Node color: " << nodeColor;
		}
		
	}
	else if ( xml.name() == "BorderStyle" ) {
		
		
	}
	else if (xml.name() == "NodeLabel" ) {
		key_value=xml.readElementText();  //see if there's simple text after the StartElement
		if (!xml.hasError()) {
			qDebug()<< "         Node Label "<< key_value;		
			nodeLabel = key_value;
		}
		else {
			qDebug()<< "         Can't read Node Label. There must be more elements nested here, continuing";  
		}
	}
	else if (xml.name() == "Shape" ) {
		if ( xml.attributes().hasAttribute("type") ) {
			nodeShape= xml.attributes().value("type").toString();	
			qDebug()<< "        Node shape: " << nodeShape;
		}
	
	}
		 

}

void Parser::readGraphMLElementEdgeGraphics(QXmlStreamReader &xml) {
	qDebug()<< "       Parser: readGraphMLElementEdgeGraphics() element name "<< xml.name();

	float tempX =-1, tempY=-1, temp=-1;
	QString color, tempString;

	if ( xml.name() == "Path" ) {

			if ( xml.attributes().hasAttribute("sx") ) {
				conv_OK=false;
				tempX = xml.attributes().value("sx").toString().toFloat (&conv_OK) ;
				if (conv_OK) 
					bez_p1_x = tempX;
				else bez_p1_x = 0 ;
			}
			if ( xml.attributes().hasAttribute("sy") ) {
				conv_OK=false;
				tempY = xml.attributes().value("sy").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					bez_p1_y = tempY;
				else bez_p1_y = 0 ;
			}
			if ( xml.attributes().hasAttribute("tx") ) {
				conv_OK=false;
				tempX = xml.attributes().value("tx").toString().toFloat (&conv_OK) ;
				if (conv_OK) 
					bez_p2_x = tempX;
				else bez_p2_x = 0 ;
			}
			if ( xml.attributes().hasAttribute("ty") ) {
				conv_OK=false;
				tempY = xml.attributes().value("ty").toString().toFloat (&conv_OK) ;
				if (conv_OK)
					bez_p2_y = tempY;
				else bez_p2_y = 0 ;
			}
			qDebug()<< "        Edge Path control points: " << bez_p1_x << " " << bez_p1_y << " " << bez_p2_x << " " << bez_p2_y;
	}
	else if (xml.name() == "LineStyle" ){
		if ( xml.attributes().hasAttribute("color") ) {
			edgeColor= xml.attributes().value("color").toString();	
			qDebug()<< "        Edge color: " << edgeColor;
		}
		if ( xml.attributes().hasAttribute("type") ) {
			edgeType= xml.attributes().value("type").toString();	
			qDebug()<< "        Edge type: " << edgeType;
		}
		if ( xml.attributes().hasAttribute("width") ) {
			temp = xml.attributes().value("width").toString().toFloat (&conv_OK) ;
			if (conv_OK)
				edgeWeight = temp;
			else 
				edgeWeight=1.0;
			qDebug()<< "        Edge width: " << edgeWeight;
		}
		
	}
	else if ( xml.name() == "Arrows" ) {
		if ( xml.attributes().hasAttribute("source") ) {
			tempString = xml.attributes().value("source").toString();	
			qDebug()<< "        Edge source arrow type: " << tempString;
		}
		if ( xml.attributes().hasAttribute("target") ) {
			tempString = xml.attributes().value("target").toString();	
			qDebug()<< "        Edge target arrow type: " << tempString;
		}

	
		
	}
	else if (xml.name() == "EdgeLabel" ) {
		key_value=xml.readElementText();  //see if there's simple text after the StartElement
		if (!xml.hasError()) {
			qDebug()<< "         Edge Label "<< key_value;		
			//probably there's more than simple text after StartElement
			edgeLabel = key_value;
		}
		else {
			qDebug()<< "         Can't read Edge Label. More elements nested ? Continuing with blank edge label....";
			edgeLabel = "" ;  
		}
	}
		 


}


void Parser::readGraphMLElementUnknown(QXmlStreamReader &xml) {
	qDebug()<< "Parser: readGraphMLElementUnknown()";
    Q_ASSERT(xml.isStartElement());
	qDebug()<< "   "<< xml.name().toString() ;
}





/**
	Tries to load a file as GML formatted network. If not it returns -1
*/
int Parser::loadGML(){
	qDebug("Parser: loadGML()");
	QFile file ( fileName );
	QString str, temp;
	int fileLine=0, start=0, end=0;
	Q_UNUSED(start);
	Q_UNUSED(end);

	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	while (!ts.atEnd() )   {
		str= ts.readLine() ;
		fileLine++;
		qDebug ()<<"Reading fileLine "<< fileLine;
		if ( fileLine == 1 ) {
			qDebug ()<<"Reading fileLine = "<< fileLine;
			if ( !str.startsWith("graph", Qt::CaseInsensitive) ) {
				qDebug() << "*** Parser:loadGML(): Not an GML-formatted file. Aborting";
				file.close();
				return -1;  
			}

		}
		if ( str.startsWith("directed",Qt::CaseInsensitive) ) { 	 //key declarations
		}
		else if ( str.startsWith("id",Qt::CaseInsensitive) ) { 	 
		}
		else if ( str.startsWith("label",Qt::CaseInsensitive) ) { 	 
		}
		else if ( str.startsWith("node",Qt::CaseInsensitive) ) { 	 //node declarations
		}
		else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { 	 //edge declarations
		}


	}
	emit fileType(5, networkName, aNodes, totalLinks, undirected);
	return 1;
}

/**
	Tries to load the file as Dot (Graphviz) formatted network. If not it returns -1
*/
int Parser::loadDot(){
	qDebug("Parser: loadDotNetwork");
	int fileLine=0, j=0, aNum=-1;
	int start=0, end=0;
	QString str, temp, label, node, nodeLabel, fontName, fontColor, edgeShape, edgeColor, edgeLabel;
	QStringList lineElement;
	nodeColor="red"; 
	edgeColor="black";
	nodeShape="";
	edgeWeight=1.0;
	float nodeValue=1.0;
	QStringList labels;
	QList<QString> nodeSequence;   //holds edges
	QList<QString> nodesDiscovered; //holds nodes;
	initShowLabels=TRUE;	
	undirected=false; arrows=TRUE; bezier=FALSE;
	source=0, target=0;
	QFile file ( fileName );
	if ( ! file.open(QIODevice::ReadOnly )) return -1;
	QTextStream ts( &file );
	aNodes=0;
	while (!ts.atEnd() )   {
		str= ts.readLine() ;
		fileLine++;
		qDebug ()<<"Reading fileLine "<< fileLine;
		if (str.isEmpty() ) continue;
		str=str.simplified();
		str=str.trimmed();
		if ( fileLine == 1 ) {
			qDebug ()<<"Reading fileLine = " <<fileLine;
			if ( str.contains("vertices",Qt::CaseInsensitive) || (str.contains("network",Qt::CaseInsensitive) || str.contains("DL",Qt::CaseInsensitive) || str.contains("list",Qt::CaseInsensitive)) || str.startsWith("<graphml",Qt::CaseInsensitive) || str.startsWith("<?xml",Qt::CaseInsensitive)) {
				qDebug() << "*** Parser:loadDot(): Not an GraphViz -formatted file. Aborting";
				file.close();				
				return -1;
			}
			if ( str.contains("digraph", Qt::CaseInsensitive) ) {
				qDebug("This is a digraph");
				//symmetricAdjacency=FALSE; 
				lineElement=str.split(" ");
				if (lineElement[1]=="{" ) networkName="Noname";
				else networkName=lineElement[1];
				continue; 
			}
			else if ( str.contains("graph", Qt::CaseInsensitive) ) {
				qDebug("This is a graph");
				//symmetricAdjacency=TRUE; 
				lineElement=str.split(" ");
				if (lineElement[1]=="{" ) networkName="Noname";
				else networkName=lineElement[1];
				continue;
			}
			else {
				qDebug()<<" *** Parser:loadDot(): Not a GraphViz file. Abort: dot format can only start with \" (di)graph netname {\"";
				return -1;  				
			}
			
		}
		if ( str.startsWith("node",Qt::CaseInsensitive) ) { 	 //Default node properties
			qDebug("Node properties found!");
			start=str.indexOf('[');
			end=str.indexOf(']');
			temp=str.right(str.size()-end-1);
			str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug()<<"Properties start at "<< start<< " and end at "<< end;
			qDebug()<<str.toAscii();
			str=str.simplified();
			qDebug()<<str.toAscii();
			start=0;
			end=str.count();

			dotProperties(str, nodeValue, nodeLabel, nodeShape, nodeColor, fontName, fontColor );

			qDebug ("Ooola! Finished node properties - let's see if there are any nodes after that!");
			temp.remove(';');
			qDebug()<<temp.toAscii();
			temp=temp.simplified();
			qDebug()<<temp.toAscii();
			if ( temp.contains(',') )
				labels=temp.split(' ');	
			else if (temp.contains(' ') )
				labels=temp.split(' ');
			for (j=0; j<(int)labels.count(); j++) {
				qDebug()<<"node label: "<<labels[j].toAscii()<<"." ;
				if (nodesDiscovered.contains(labels[j])) {qDebug("discovered"); continue;}
				aNodes++;
				randX=rand()%gwWidth;
				randY=rand()%gwHeight;
				qDebug()<<"Creating node at "<< randX<<","<< randY<<" label " << labels[j].toAscii(); 
				emit createNode(aNodes, initNodeSize, nodeColor, labels[j], nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
				aNum=aNodes;
				nodesDiscovered.push_back( labels[j]);
				qDebug()<<" Total aNodes: "<<  aNodes<< " nodesDiscovered = "<< nodesDiscovered.size();
			}
		}
		else if ( str.startsWith("edge",Qt::CaseInsensitive) ) { //Default edge properties
			qDebug("Edge properties found...");
			start=str.indexOf('[');
			end=str.indexOf(']');
			str=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug()<<"Properties start at "<< start <<" and end at "<< end;
			qDebug()<<str.toAscii();
			str=str.simplified();
			qDebug()<<str.toAscii();
			start=0;
			end=str.count();
			qDebug ("Finished the properties!");
		}
		//ti ginetai an grafei p.x. "node-1" -> "node-2"
		//ti ginetai an exeis mesa sxolia ? p.x. sto telos tis grammis //
		else if (str.contains('-',Qt::CaseInsensitive)) {  
			qDebug("Edge found...");
			end=str.indexOf('[');
			if (end!=-1) {
				temp=str.right(str.size()-end-1); //keep the properties
				temp=temp.remove(']');
				qDebug()<<"edge properties "<<temp.toAscii();
				dotProperties(temp, edgeWeight, edgeLabel, edgeShape, edgeColor, fontName, fontColor );
			}
			else end=str.indexOf(';');
			//FIXME It cannot parse nodes with names containing the '-' character!!!!
			str=str.mid(0, end).remove('\"');  //keep only edges
			qDebug()<<"edges "<<str.toAscii();
			
			if (!str.contains("->",Qt::CaseInsensitive)){  //non directed = symmetric links
				nodeSequence=str.split("-");
			}
			else { 		//directed
				nodeSequence=str.split("->");
			}
			for ( QList<QString>::iterator it=nodeSequence.begin(); it!=nodeSequence.end(); it++ )  {
				node=(*it).simplified();
				if ( (aNum=nodesDiscovered.indexOf( node ) ) == -1) {
					aNodes++;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<"Creating node at "<< randX <<","<< randY<<" label "<<node.toAscii(); 
					emit createNode(aNodes, initNodeSize, nodeColor, node , nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
					nodesDiscovered.push_back( node  );
					qDebug()<<" Total aNodes " << aNodes<< " nodesDiscovered  "<< nodesDiscovered.size() ;
					target=aNodes;
					if (it!=nodeSequence.begin()) {
						qDebug()<<"Drawing Link between node "<< source<< " and node " <<target;
						emit createEdge(source,target, edgeWeight, edgeColor, undirected, arrows, bezier);
					}
				}
				else {
					target=aNum+1;
					qDebug("Node already exists. Vector num: %i ",target);
					if (it!=nodeSequence.begin()) {
						qDebug()<<"Drawing Link between node "<<source<<" and node " << target;
						emit createEdge(source,target, edgeWeight , edgeColor, undirected, arrows, bezier);
					}
				}
				source=target;
			}
			nodeSequence.clear();
			qDebug("Finished reading fileLine %i ",fileLine);
		}
		else if ( str.contains ("[",Qt::CaseInsensitive) ) { 	 //Default node properties
			qDebug("Node properties found but with no Node keyword in the beggining!");
			start=str.indexOf('[');
			end=str.indexOf(']');
			temp=str.mid(start+1, end-start-1);  //keep whatever is inside [ and ]
			qDebug()<<"Properties start at "<< start<< " and end at " << end;
			temp=temp.simplified();
			qDebug()<<temp.toAscii();
			dotProperties(temp, nodeValue, label, nodeShape, nodeColor, fontName, fontColor );
			qDebug ("Finished the properties!");

			if (start > 2 ) {//there is a node definition here
				node=str.left(start).remove('\"').simplified();
				qDebug()<<"node label: "<<node.toAscii()<<"." ;
				if (!nodesDiscovered.contains(node)) {
					qDebug("not discovered node"); 
					aNodes++;
					randX=rand()%gwWidth;
					randY=rand()%gwHeight;
					qDebug()<<"Creating node at "<<  randX << " "<< randY<< " label "<<node.toAscii(); 
					emit createNode(aNodes, initNodeSize, nodeColor, label, nodeColor, QPointF(randX,randY), nodeShape, initShowLabels);
					aNum=aNodes;
					nodesDiscovered.push_back( node);
					qDebug()<<" Total aNodes: "<<  aNodes<< " nodesDiscovered = "<< nodesDiscovered.size();
				}
				else {
					qDebug("discovered node - skipping it!");
				}
			}
		}
	}
  	file.close();
	/** 
		0 means no file, 1 means Pajek, 2 means Adjacency etc	
	**/
	emit fileType(3, networkName, aNodes, totalLinks, undirected);
	return 1;
}





void Parser::dotProperties(QString str, float &nValue, QString &label, QString &shape, QString &color, QString &fontName, QString &fontColor ){
	int next=0;
	QString prop, value;	
	bool ok=FALSE;
			do  {		//stops when it passes the index of ']'
				next=str.indexOf('=', 1);
				qDebug("Found next = at %i. Start is at %i", next, 1);
				prop=str.mid(0, next).simplified();	
				qDebug()<<"Prop: "<<prop.toAscii() ;
				str=str.right(str.count()-next-1).simplified();
				qDebug()<<"whatsleft: "<<str.toAscii() ;
				if ( str.indexOf('\"') == 0) {
					qDebug("found text, parsing...");
					next=str.indexOf('\"', 1);
					value=str.left(next).simplified().remove('\"');
					
					if (prop=="label") {
						qDebug()<<"Found label "<<value.toAscii();
						label=value.trimmed();
						qDebug()<<"Assigned label "<<label.toAscii();
					}
					else if (prop=="fontname"){
						qDebug()<<"Found fontname"<<value.toAscii();
						fontName=value.trimmed();
					}
					str=str.right(str.count()-next-1).simplified();
					qDebug()<<"whatsleft: "<<str.toAscii() <<".";
				}
				else {
					if (str.isEmpty()) break;
					if ( str.contains(',') )
						next=str.indexOf(',');
					else if ( str.contains(' ') )
						next=str.indexOf(' ');
					value=str.mid(0, next).simplified();
					
					qDebug()<<"Prop Value: "<<value.toAscii() ;
					if (prop=="value") {
						qDebug()<<"Found value "<<value.toAscii();
						nValue=value.trimmed().toFloat(&ok);
						if ( ok ) 
							qDebug()<<"Assigned value "<<nValue;
						else 
							 qDebug()<<"Error in value conversion ";
					}
					else if (prop=="color") {
						qDebug()<<"Found color "<<value.toAscii();
						color=value.trimmed();
						qDebug()<<"Assigned node color "<<color.toAscii()<<".";
					}
					else if (prop=="fontcolor") {
						qDebug()<<"Found fontcolor "<<value.toAscii();
						fontColor=value.trimmed();
					}
					else if (prop=="shape") {
						shape=value.trimmed();
						qDebug()<<"Found node shape "<<shape.toAscii();
					}
					qDebug()<<"count"<< str.count()<<  " next "<< next;
					str=str.right(str.count()-next).simplified();
					qDebug()<<"whatsleft: "<<str.toAscii()<<".";
					if ( (next=str.indexOf('=', 1))==-1) break;
				}
			} while (!str.isEmpty());

}


/** starts the new thread calling the load* methods
*/

void Parser::run()  {
	qDebug("**** QThread/Parser: This is a thread, running!");
	if (networkName=="") networkName="Unnamed!";
	if ( loadPajek() ==1 ) {
		qDebug("Parser: this is a Pajek network");
	}
	else if (loadAdjacency()==1 ) {
		qDebug("Parser: this is an adjacency-matrix network");
	}
	else if (loadDot()==1 ) {
		qDebug("Parser: this is an GraphViz (dot) network");
	}    
	else if (loadGraphML()==1){
		qDebug("Parser: this is an GraphML network");
	}
	else if (loadGML()==1){
		qDebug("Parser: this is an GML (gml) network");
	}
	else if (loadDL()==1){
		qDebug("Parser: this is an DL formatted (.dl) network");
	}
	else{
	qDebug("**** QThread/Parser: end of routine!");
	}
}
