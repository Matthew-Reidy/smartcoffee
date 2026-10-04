//
//  ContentView.swift
//  coffee-app

import SwiftUI

struct ContentView: View {
    @State var socketManager = Websocket()

    private let espresso = Color(red: 0.15, green: 0.08, blue: 0.04)
    private let roast    = Color(red: 0.25, green: 0.14, blue: 0.07)
    private let cream    = Color(red: 0.98, green: 0.89, blue: 0.72)
    private let gold     = Color(red: 0.98, green: 0.75, blue: 0.30)
    private let crimson  = Color(red: 0.70, green: 0.20, blue: 0.10)

    var body: some View {
        ZStack {
            LinearGradient(colors: [espresso, roast], startPoint: .top, endPoint: .bottom)
                .ignoresSafeArea()

            VStack(spacing: 48) {
                // Header
                VStack(spacing: 10) {
                    Text("Smart Coffee")
                        .font(.system(size: 38, weight: .bold, design: .rounded))
                        .foregroundColor(cream)

                    HStack(spacing: 6) {
                        Circle()
                            .fill(socketManager.isConnected ? Color.green : Color.gray)
                            .frame(width: 8, height: 8)
                        Text(socketManager.isConnected ? "Connected" : "Disconnected")
                            .font(.caption)
                            .foregroundColor(.white.opacity(0.55))
                    }
                }

                // Coffee cup + steam
                VStack(spacing: 0) {
                    HStack(spacing: 10) {
                        ForEach(0..<3, id: \.self) { i in
                            SteamLine(isVisible: socketManager.isBrewActive, delay: Double(i) * 0.4)
                        }
                    }
                    .frame(height: 52)

                    Image(systemName: "cup.and.saucer.fill")
                        .font(.system(size: 110))
                        .foregroundStyle(socketManager.isBrewActive ? gold : cream.opacity(0.5))
                        .animation(.easeInOut(duration: 0.4), value: socketManager.isBrewActive)
                }

                // Brew button
                Button {
                    let next = !socketManager.isBrewActive
                    socketManager.isBrewActive = next
                    socketManager.sendBrew(active: next)
                } label: {
                    Text(socketManager.isBrewActive ? "Stop Brewing" : "Brew Now")
                        .font(.system(size: 22, weight: .semibold, design: .rounded))
                        .foregroundColor(socketManager.isBrewActive ? .white : espresso)
                        .frame(width: 220, height: 60)
                        .background(socketManager.isBrewActive ? crimson : gold)
                        .clipShape(Capsule())
                        .shadow(color: .black.opacity(0.35), radius: 10, x: 0, y: 5)
                }
                .disabled(!socketManager.isConnected)
                .opacity(socketManager.isConnected ? 1 : 0.45)
                .animation(.easeInOut(duration: 0.3), value: socketManager.isBrewActive)

                // Status label
                Text(socketManager.isBrewActive ? "Brewing in progress…" : " ")
                    .font(.system(size: 15, design: .rounded))
                    .foregroundColor(gold.opacity(0.8))
                    .animation(.easeInOut, value: socketManager.isBrewActive)
            }
            .padding(.horizontal, 32)
        }
        .onAppear  { socketManager.connect() }
        .onDisappear { socketManager.disconnect() }
    }
}

// MARK: – Steam animation

struct SteamLine: View {
    let isVisible: Bool
    let delay: Double
    @State private var animating = false

    var body: some View {
        Capsule()
            .fill(Color.white.opacity(animating ? 0 : 0.55))
            .frame(width: 5, height: 22)
            .offset(y: animating ? -38 : 0)
            .opacity(isVisible ? 1 : 0)
            .onAppear { startLoop() }
            .onChange(of: isVisible) { _, visible in
                if visible { animating = false; startLoop() }
            }
    }

    private func startLoop() {
        withAnimation(
            Animation.easeOut(duration: 1.4)
                .repeatForever(autoreverses: false)
                .delay(delay)
        ) {
            animating = true
        }
    }
}

#Preview {
    ContentView()
}
