//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "SYNACKClassifier.h"

Define_Module(SYNACKClassifier);

int SYNACKClassifier::classifyPacket(Packet *packet)
{
    if (strcmp(packet->getName(), "SYN") == 0 || strcmp(packet->getName(), "TcpAck") == 0){
        if(strcmp(packet->getName(), "SYN") == 0)
            std::cout << "SYN Ack stored in queue" << std::endl;
        auto outputConsumer = consumers[0];
        if(outputConsumer->canPushSomePacket(outputGates[0]));
            return 0;
    } else{
        auto outputConsumer = consumers[1];
        if(outputConsumer->canPushSomePacket(outputGates[1]));
            return 1;
    }
    return -1;
}


