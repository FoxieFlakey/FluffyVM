package com.caiomgt.login;


import org.bukkit.ChatColor;
import org.bukkit.command.Command;
import org.bukkit.command.CommandExecutor;
import org.bukkit.command.CommandSender;
import org.bukkit.entity.Player;
import org.bukkit.plugin.Plugin;

import java.util.Set;

public class login implements CommandExecutor {
    Plugin plugin;
    public login(Plugin plugin){
        this.plugin = plugin;
    }
    @Override
    public boolean onCommand(CommandSender sender, Command cmd, String label, String[] args){
        if (!(sender instanceof Player)){
            return true;
        }
        Player plr = (Player) sender;
        if (cmd.getName().equalsIgnoreCase("register")){
            Set<String> tags = plr.getScoreboardTags();
            for (String tag : tags){
                if (!tag.equals("noLogin")){
                    continue;
                }
                if (args.length != 2){
                    plr.sendMessage(ChatColor.YELLOW + "Usage: /register password password");
                    continue;
                }
                if (args[0].length() < 3 || args[1].length() < 3){
                    continue;
                }
                if (!args[0].equals(args[1])){
                    plr.sendMessage(ChatColor.YELLOW + "The passwords need to match");
                    continue;
                }
                plr.sendMessage(ChatColor.YELLOW+ "Registered! Use /login to enter.");
                plugin.getConfig().set("plr"+plr.getName(), args[0]);
                plugin.saveConfig();
            }
        } else if (cmd.getName().equalsIgnoreCase("login")){
            boolean shouldSetOp;
            Set<String> tags = plr.getScoreboardTags();
            for (String tag : tags){
                if (!tag.equals("noLogin"))
                    continue;
                if (args.length != 1){
                    plr.sendMessage(ChatColor.YELLOW + "Usage: /login password");
                    continue;
                }
                String pass = plugin.getConfig().getString("plr"+plr.getName());
                if (pass == null){
                    plr.sendMessage(ChatColor.YELLOW + "Use /register to set a password");
                    continue;
                }
                if (!pass.equals(args[0])){
                    continue;
                }
                plr.sendMessage(ChatColor.YELLOW + "Login successful!");
                plr.removeScoreboardTag("noLogin");
                plr.setAllowFlight(false);
            }
        }
        return true;
    }
}
