///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016 F4EXB                                                      //
// written by Edouard Griffiths                                                  //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDE_GUI_GLSHADERSOURCES_H_
#define INCLUDE_GUI_GLSHADERSOURCES_H_

#include <QString>

class GLShaderSources
{
public:
	static const QString& getVertexShaderSourceSimple() { return m_vertexShaderSourceSimple; }
	static const QString& getFragmentShaderSourceColored() { return m_fragmentShaderSourceColored; }
private:
	static const QString m_vertexShaderSourceSimple;
	static const QString m_fragmentShaderSourceColored;
};

#endif /* INCLUDE_GUI_GLSHADERSOURCES_H_ */