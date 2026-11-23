#import <Cocoa/Cocoa.h>

static id monitorEscape = nil;

// Ambos bucles distribuyen eventos Cocoa. Registrar Escape antes del destinatario
// evita perderlo si VTK lo retira de la cola antes de waitKey del Panel.
void observarEscape(bool* salir){
    monitorEscape = [[NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
        handler:^NSEvent* (NSEvent* evento){
            if(evento.keyCode == 53){ // Tecla física Escape en macOS.
                *salir = true;
            }
            return evento;
        }] retain];
}

void dejarDeObservarEscape(){
    [NSEvent removeMonitor:monitorEscape];
    [monitorEscape release];
    monitorEscape = nil;
}
