//
//  ContentView.swift
//  coffee-app


import SwiftUI



struct ContentView: View {
    
    @State public var brewActive = false
    @State private var buttonState = "Brew Now!"
    @State var socketManager = Websocket()
    
    var body: some View {
        
        VStack{
            Text("Coffee Control Pannel")
                .font(.largeTitle)
                .padding()
        }
        
        VStack {

            Button(buttonState){
                
                if(brewActive){
                    buttonState = "End Brew"
                    brewActive = false
                    socketManager.sendMessage(message: "test1")
                }else{
                    buttonState = "Brew Now!"
                    brewActive = true
                    socketManager.sendMessage(message: "test2")
                }
                
            }.font(.system(size: 25))
             .buttonStyle(.borderedProminent)
        }.onAppear{
            socketManager.connect()
        }.onDisappear(){
            socketManager.disconnect()
        }
        .padding()
    }
}

#Preview {
    ContentView()
}
