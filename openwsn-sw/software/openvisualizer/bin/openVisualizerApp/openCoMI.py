from threading import Thread
from   cmd import Cmd
import logging
import os
import sys
import struct
log = logging.getLogger('openCoMI')

from openvisualizer.openType      import openType,         \
                                         typeAsn,          \
                                         typeAddr,         \
                                         typeCellType,     \
                                         typeComponent,    \
                                         typeRssi
                
                
############################### Related to CBOR                
from cbor.cbor import dumps as pydumps
from cbor.cbor import loads as pyloads
from cbor.cbor import dump as pydump
from cbor.cbor import load as pyload
from cbor.cbor import Tag
try:
    from cbor._cbor import dumps as cdumps
    from cbor._cbor import loads as cloads
    from cbor._cbor import dump as cdump
    from cbor._cbor import load as cload
except ImportError:
    # still test what we can without C fast mode
    logger.warn('testing without C accelerated CBOR', exc_info=True)
    cdumps, cloads, cdump, cload = None, None, None, None


_IS_PY3 = sys.version_info[0] >= 3


if _IS_PY3:
    _range = range
    from io import BytesIO as StringIO
else:
    _range = xrange
    from cStringIO import StringIO

############################## Related to Mote State   
try:
    from openvisualizer.moteState import moteState
except ImportError:
    # Debug failed lookup on first library import
    print 'ImportError: cannot find openvisualizer.moteState module'
    print 'sys.path:\n\t{0}'.format('\n\t'.join(str(p) for p in sys.path))

try:
    from twisted.internet import defer
    from twisted.internet.protocol import DatagramProtocol
    from twisted.internet import reactor
    import openvisualizer.txthings.resource as resource
    import openvisualizer.txthings.coap as coap
    import openvisualizer.txthings.error as error
except ImportError:
    # Debug failed lookup on first library import
    print 'ImportError: cannot find modules for coap operations'


class openCoMI(Cmd):
    
    def __init__(self, app):

        self.app = app        
        root = resource.CoAPResource()
        well_known = resource.CoAPResource()
        root.putChild('.well-known', well_known)
        core = CoreResource(root)
        well_known.putChild('core', core)
                   
        comi = resource.CoAPResource()
        root.putChild('c', comi)  
              
        info = InfoResource(5000)
        root.putChild('info', info)
                                
        cellList = CellListResource(self.app)
        comi.putChild('+i', cellList)
        
        neighborList = NeighborResource(self.app)
        comi.putChild('+r', neighborList)
                
        routeList = RouteResource(self.app)
        comi.putChild('+1', routeList)
        
        print('CoMI Resources are created')
                              
        endpoint = resource.Endpoint(root)
        reactor.listenUDP(coap.COAP_PORT, coap.Coap(endpoint),interface="bbbb::1") #, interface="::")
        return
        
    def start(self):
        #  reactor.run(installSignalHandlers=0)    
        self.mythread = Thread(target=reactor.run, args=(False,)).start()
        return 
        
    def stop(self):
        #  reactor.run(installSignalHandlers=0)    
        reactor.stop()
        return 


class InfoResource (resource.CoAPResource):

    def __init__(self, start=0):
        resource.CoAPResource.__init__(self)
        self.counter = start
        self.visible = True
        self.addParam(resource.LinkParam("title", "Info"))

    def render_GET(self, request):
       # response = coap.Message(code=coap.CONTENT, payload='%d' % (self.counter,))
       response = coap.Message(code=coap.CONTENT, payload='This is a BBR for a 6TSCH Network(bbbb::) with CoMI based 6TiSCH Resources')
       # self.counter += 1
       return defer.succeed(response)


class CoreResource(resource.CoAPResource):
    def __init__(self, root):
        resource.CoAPResource.__init__(self)
        self.root = root

    def render_GET(self, request):
        data = []
        self.root.generateResourceList(data, "")
        payload = ",".join(data)
        print payload
        response = coap.Message(code=coap.CONTENT, payload=payload)
        response.opt.content_format = coap.media_types_rev['application/link-format']
        return defer.succeed(response)

class CellListResource(resource.CoAPResource):
    
    def __init__(self,app):
        resource.CoAPResource.__init__(self)
        
        self.app=app        
        self.visible = True
        self.addParam(resource.LinkParam("title", "CellListResource"))
            
    def render_GET(self, request):
      #  response = coap.Message(code=coap.CONTENT, payload='%d' % (moteState.ST_OUPUTBUFFER,))
        payload_array = []
        
        for ms in self.app.moteStates:
            try:
                if ms.getStateElem('IdManager').data[0]['isDAGroot'] == 1:
                    curr_asn_str = str(ms.getStateElem('Asn').data[0]['asn'])
                    print('Reading CellList Info from Border Router at ASN:' + curr_asn_str)
                    
                    cellList=ms.getStateElem('Schedule').data

                    slot_frame_id = 1;
                    
                    for cell in cellList:
                        celltype=cell.data[0]['type'].get()                        
                        if celltype > 0:
                                            
                            info = cell.data[0]
                            cell_id = info['cellID'] 
                            slotOffset = info['slotOffset'] 
                            channelOffset = info['channelOffset']                           
                            celltype=info['type'].get()          
                            shared = info['shared']       
                            ishard = info['isHard']
                            label = info['label']   
                            neighboraddress =info['neighbor'].get()          
                            if len(neighboraddress)>2:
                                neighbor = struct.pack('!B',int(neighboraddress[-2:],16))  
                            else:
                                neighbor = neighboraddress
                            
                            txack=info['numTxACK']
                            tx  = info['numTx']   
                            linkoption = self.getLinkOption(celltype,shared,ishard,label)                                                     
                                                
                            if tx == 0:
                                stat=100
                            else: 
                                stat=(100 * txack) / tx 
                
                            if celltype < 4:     # if serial asn will be 0                             
                                asn_diff = int(curr_asn_str, 16) - int(str(info['lastUsedAsn']),16)
                            else:
                                asn_diff = 0    
                            
                            cell = [{1 : cell_id, 2: slot_frame_id, 3: slotOffset, 4: channelOffset, 5: linkoption, 6: neighbor, 7:stat, 8: asn_diff}]
                            payload_array += cell 
                              
                            
            except ValueError as err:
                print(err)
       # print(mypayload)
       
        fob = StringIO()
        pydump(payload_array, fob)  
        fob.seek(0)
       
        response = coap.Message(code=coap.CONTENT, payload=fob.getvalue())
        response.opt.content_format = coap.media_types_rev['application/cbor']
        return defer.succeed(response)

    def render_PUT(self, request):
        try:
            parsed_payload = pyloads(request.payload)    
            if len(request.opt.uri_query)>0:
                queryText=request.opt.uri_query[0]
                if(len(queryText)>2):
                    key = 0
                    if (queryText[:2]) == 'k=':
                        key=request.opt.uri_query[0][2:]
                    else:
                        response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                        return defer.succeed(response)
                    
                    print("Received a put request for the border router")
                    for ms in self.app.moteStates:
                        try:
                            if ms.getStateElem('IdManager').data[0]['isDAGroot'] == 1:
                                cellID          = parsed_payload[1]
                                slotframeID     = parsed_payload[2]
                                slotoffset      = parsed_payload[3]
                                channeloffset   = parsed_payload[4]
                                celloptions     = ord(parsed_payload[5])
                                address         = ord(parsed_payload[6])
                                print("Adding A new Cell for Border Router at key " + key)                           
                                ms.triggerAction([moteState.moteState.COMI_COMMAND,moteState.moteState.COMMAND_COMI_ADD[0],cellID,slotframeID,slotoffset,channeloffset,celloptions,address])
                                payload = "Cell is Added!! @" + key
                                response = coap.Message(code=coap.CHANGED, payload=payload)
                                return defer.succeed(response)
                        except ValueError as err:
                            print(err)
                    response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                    return defer.succeed(response)   
                else:
                    response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                    return defer.succeed(response)
            else:
                response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                return defer.succeed(response)
        except ValueError as err:
                print(err)
                
    def render_DELETE(self, request):
        try:
            if len(request.opt.uri_query)>0:
                queryText=request.opt.uri_query[0]
                if(len(queryText)>2):
                    key = 0
                    if (queryText[:2]) == 'k=':
                        key=request.opt.uri_query[0][2:]
                    else:
                        response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                        return defer.succeed(response)
                    
                    print("Deleting the Cell for Border Router at CellID " + key)
                    for ms in self.app.moteStates:
                        try:
                            if ms.getStateElem('IdManager').data[0]['isDAGroot'] == 1:                                                              
                                ms.triggerAction([moteState.moteState.COMI_COMMAND,moteState.moteState.COMMAND_COMI_DELETE[0],key])
                                payload = "Cell is Deleted!! @" + key
                                response = coap.Message(code=coap.DELETED, payload=payload)
                                return defer.succeed(response)
                        except ValueError as err:
                            print(err)
                    response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                    return defer.succeed(response)   
                else:
                    response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                    return defer.succeed(response)
            else:
                response = coap.Message(code=coap.METHOD_NOT_ALLOWED)
                return defer.succeed(response)
        except ValueError as err:
                print(err)

    def getLinkOption(self,celltype, shared, ishard, label):

        linkoption=0x00
        
        if celltype == 1: #typeCellType.CELLTYPE_TX:
            linkoption=linkoption + 0x80                  
        elif celltype == 2: #typeCellType.CELLTYPE_RX:
            linkoption=linkoption + 0x40  
        elif celltype == 3: #typeCellType.CELLTYPE_TXRX:
            linkoption=linkoption + 0x80 + 0x40 
       
        if shared == 1:
            linkoption=linkoption + 0x20 
        if ishard == 1:
            linkoption=linkoption + 0x04   
        if label > 0:
            linkoption=linkoption + 0x08   
        return struct.pack('!B', linkoption)

class NeighborResource(resource.CoAPResource):
    
    def __init__(self,app):
        resource.CoAPResource.__init__(self)
        
        self.app=app        
        self.visible = True
        self.addParam(resource.LinkParam("title", "NeigborListResource"))

    def render_GET(self, request):
      #  response = coap.Message(code=coap.CONTENT, payload='%d' % (moteState.ST_OUPUTBUFFER,))
        payload_array = []
        for ms in self.app.moteStates:
            try:
                if ms.getStateElem('IdManager').data[0]['isDAGroot'] == 1:
                    curr_asn_str = str(ms.getStateElem('Asn').data[0]['asn'])
                    print('Reading Neighbor Info from Border Router at ASN:' + curr_asn_str)
                    neighborList=ms.getStateElem('Neighbors').data
                    neighbor_id = 0
                    for neighbor in neighborList:
                        if neighbor.data[0]['used']==1:
                        
                            info = neighbor.data[0]
    
                            address = struct.pack('!B',int(info['addr'].get()[-2:],16))
                            rssi =  int(info['rssi'].get())
                      
                            txack=info['numTxACK']
                            tx  = info['numTx']   
                            if tx == 0:
                                stat=100
                            else: 
                                stat=(100 * txack) / tx       
                                                     
                            asn_diff = int(curr_asn_str, 16) - int(str(info['asn']),16)
                            neighbor = [{1 : neighbor_id, 2: address, 3: rssi, 4: stat, 5: asn_diff}]
                            payload_array += neighbor
                            neighbor_id = neighbor_id + 1
            except ValueError as err:
                print(err)
       # print(mypayload)
       
        fob = StringIO()
        pydump(payload_array, fob)  
        fob.seek(0)
        
        response = coap.Message(code=coap.CONTENT, payload=fob.getvalue())
        response.opt.content_format = coap.media_types_rev['application/cbor']
        return defer.succeed(response)

class RouteResource(resource.CoAPResource):
    
    def __init__(self,app):
        resource.CoAPResource.__init__(self)
        
        self.app=app        
        self.visible = True
        self.addParam(resource.LinkParam("title", "RouteResource"))

    def render_GET(self, request):
      #  response = coap.Message(code=coap.CONTENT, payload='%d' % (moteState.ST_OUPUTBUFFER,))
        payload_array = []
        for ms in self.app.moteStates:
            try:
                if ms.getStateElem('IdManager').data[0]['isDAGroot'] == 1:
                    curr_asn_str = str(ms.getStateElem('Asn').data[0]['asn'])
                    print('Reading Route Info from Border Router at ASN:' + curr_asn_str)
                    route = {}
                    payload_array += route

            except ValueError as err:
                print(err)
        fob = StringIO()
        pydump(payload_array, fob)  
        fob.seek(0)
        print(fob.getvalue())
       
        response = coap.Message(code=coap.CONTENT, payload=fob.getvalue())
        response.opt.content_format = coap.media_types_rev['application/cbor']
        return defer.succeed(response)
