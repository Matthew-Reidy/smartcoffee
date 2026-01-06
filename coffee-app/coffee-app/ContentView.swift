//
//  ContentView.swift
//  coffee-app


import SwiftUI



struct ContentView: View {
    
    @State public var brewActive = false
    @State private var buttonState = "Brew Now!"
    
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
                }else{
                    buttonState = "Brew Now!"
                    brewActive = true
                }
                
            }.font(.system(size: 25))
             .buttonStyle(.borderedProminent)
        }

        .padding()
    }
}

#Preview {
    ContentView()
}
