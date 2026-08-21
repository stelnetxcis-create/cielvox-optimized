package com.stelnet_asr.demo

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.viewModels
import com.stelnet_asr.demo.ui.main.MainScreen
import com.stelnet_asr.demo.ui.main.MainScreenViewModel
import com.stelnet_asr.demo.ui.theme.StelnetASRDemoTheme

class MainActivity : ComponentActivity() {
    private val viewModel: MainScreenViewModel by viewModels { MainScreenViewModel.factory() }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            StelnetASRDemoTheme {
                MainScreen(viewModel)
            }
        }
    }
}
